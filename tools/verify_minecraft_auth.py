"""One-shot public-client PKCE interoperability probe; no product credentials are saved."""

from __future__ import annotations

import argparse
import base64
from dataclasses import dataclass, field
import hashlib
import hmac
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import os
from pathlib import Path
import re
import secrets
import socket
import time
from urllib.error import HTTPError, URLError
from urllib.parse import parse_qs, urlencode, urlsplit
from urllib.request import HTTPRedirectHandler, Request, build_opener
import uuid

AUTHORIZE = "https://login.microsoftonline.com/consumers/oauth2/v2.0/authorize"
TOKEN = "https://login.microsoftonline.com/consumers/oauth2/v2.0/token"
USER = "https://user.auth.xboxlive.com/user/authenticate"
XSTS = "https://xsts.auth.xboxlive.com/xsts/authorize"
MINECRAFT = "https://api.minecraftservices.com/authentication/login_with_xbox"
ENTITLEMENTS = "https://api.minecraftservices.com/entitlements/mcstore"
PROFILE = "https://api.minecraftservices.com/minecraft/profile"
ENDPOINTS = frozenset((TOKEN, USER, XSTS, MINECRAFT, ENTITLEMENTS, PROFILE))


class ProbeFailure(Exception):
    def __init__(self, code: str, http_status: int | None = None):
        super().__init__(code)
        self.code = code
        self.http_status = http_status


@dataclass
class Authorization:
    client_id: str
    redirect_uri: str
    state: str = field(repr=False)
    verifier: str = field(repr=False)

    def url(self) -> str:
        challenge = base64.urlsafe_b64encode(hashlib.sha256(self.verifier.encode("ascii")).digest()).rstrip(b"=").decode()
        return AUTHORIZE + "?" + urlencode({
            "client_id": self.client_id, "response_type": "code", "response_mode": "query",
            "redirect_uri": self.redirect_uri, "scope": "XboxLive.signin",
            "code_challenge": challenge, "code_challenge_method": "S256", "state": self.state,
        })


def callback_result(path: str, host: str, auth: Authorization) -> tuple[str, str]:
    expected = urlsplit(auth.redirect_uri)
    parsed = urlsplit(path)
    if host != expected.netloc or parsed.path != "/" or parsed.scheme or parsed.netloc or parsed.fragment:
        raise ProbeFailure("invalid_callback")
    if len(path) > 16384:
        raise ProbeFailure("invalid_callback")
    try:
        params = parse_qs(parsed.query, keep_blank_values=True, max_num_fields=12)
    except ValueError:
        raise ProbeFailure("invalid_callback") from None
    if len(params.get("state", [])) != 1 or not hmac.compare_digest(params["state"][0].encode("utf-8"), auth.state.encode("utf-8")):
        raise ProbeFailure("invalid_callback_state")
    if "code" in params and "error" not in params and len(params["code"]) == 1 and params["code"][0]:
        return "code", params["code"][0]
    if "error" in params and "code" not in params and len(params["error"]) == 1:
        return "error", "access_denied" if params["error"][0] == "access_denied" else "authorization_failed"
    raise ProbeFailure("invalid_callback")


def remote_error(data: object) -> str:
    if not isinstance(data, dict):
        return "remote_error"
    for key in ("XErr",):
        number = data.get(key)
        if type(number) is int and 0 <= number < 10**12:
            return f"xbox_{number}"
    codes = data.get("error_codes")
    if isinstance(codes, list) and codes and type(codes[0]) is int and 0 <= codes[0] < 10**9:
        return f"aadsts_{codes[0]}"
    if isinstance(data.get("error"), str) and data["error"] in {"invalid_grant", "invalid_client", "invalid_scope", "access_denied", "unauthorized_client"}:
        return data["error"]
    if any(isinstance(data.get(key), str) and "invalid app registration" in data[key].lower()
           for key in ("errorMessage", "message")):
        return "invalid_app_registration"
    return "remote_error"


class NoRedirect(HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        raise ProbeFailure("redirect_refused", code)


class LoopbackServer(HTTPServer):
    allow_reuse_address = False

    def server_bind(self):
        if hasattr(socket, "SO_EXCLUSIVEADDRUSE"):
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)
        super().server_bind()


def request_json(url: str, payload: dict | None = None, bearer: str | None = None) -> dict:
    if url not in ENDPOINTS:
        raise ProbeFailure("unapproved_endpoint")
    headers = {"Accept": "application/json", "User-Agent": "HustMCL-AuthProbe/0.1"}
    body = None
    if payload is not None:
        if url == TOKEN:
            body = urlencode(payload).encode("ascii")
            headers["Content-Type"] = "application/x-www-form-urlencoded"
        else:
            body = json.dumps(payload).encode("utf-8")
            headers["Content-Type"] = "application/json"
    if url in (USER, XSTS):
        headers["x-xbl-contract-version"] = "1"
    if bearer is not None:
        headers["Authorization"] = "Bearer " + bearer
    try:
        with build_opener(NoRedirect()).open(Request(url, data=body, headers=headers), timeout=25) as response:
            raw = response.read(1024 * 1024 + 1)
    except HTTPError as error:
        try:
            data = json.loads(error.read(65536))
        except (ValueError, UnicodeError):
            data = None
        raise ProbeFailure(remote_error(data), error.code) from None
    except (URLError, TimeoutError, OSError):
        raise ProbeFailure("network_error") from None
    if len(raw) > 1024 * 1024:
        raise ProbeFailure("response_too_large")
    try:
        data = json.loads(raw)
    except (ValueError, UnicodeError):
        raise ProbeFailure("invalid_response") from None
    if not isinstance(data, dict):
        raise ProbeFailure("invalid_response")
    return data


def token_string(data: dict, key: str) -> str:
    value = data.get(key)
    if not isinstance(value, str) or not value or any(ord(ch) < 32 for ch in value):
        raise ProbeFailure("invalid_response")
    return value


def xbox_claim(data: dict) -> tuple[str, str]:
    token = token_string(data, "Token")
    try:
        uhs = data["DisplayClaims"]["xui"][0]["uhs"]
    except (KeyError, IndexError, TypeError):
        raise ProbeFailure("invalid_response") from None
    if not isinstance(uhs, str) or not re.fullmatch(r"[0-9]+", uhs):
        raise ProbeFailure("invalid_response")
    return token, uhs


def verify_chain(auth: Authorization, code: str, report: dict, send=request_json) -> None:
    stage = "microsoft_token"
    try:
        microsoft = send(TOKEN, {"client_id": auth.client_id, "code": code, "redirect_uri": auth.redirect_uri,
                                "grant_type": "authorization_code", "code_verifier": auth.verifier})
        access = token_string(microsoft, "access_token")
        report["stages"].append({"stage": stage, "status": "succeeded"})
        stage = "xbox_user"
        user = send(USER, {"RelyingParty": "http://auth.xboxlive.com", "TokenType": "JWT", "Properties": {
            "AuthMethod": "RPS", "SiteName": "user.auth.xboxlive.com", "RpsTicket": "d=" + access}})
        user_token, user_hash = xbox_claim(user)
        report["stages"].append({"stage": stage, "status": "succeeded"})
        stage = "xsts"
        xsts = send(XSTS, {"RelyingParty": "rp://api.minecraftservices.com/", "TokenType": "JWT", "Properties": {
            "SandboxId": "RETAIL", "UserTokens": [user_token]}})
        xsts_token, xsts_hash = xbox_claim(xsts)
        if not hmac.compare_digest(user_hash, xsts_hash):
            raise ProbeFailure("user_hash_changed")
        report["stages"].append({"stage": stage, "status": "succeeded"})
        stage = "minecraft_token"
        minecraft = send(MINECRAFT, {"identityToken": f"XBL3.0 x={user_hash};{xsts_token}"})
        minecraft_token = token_string(minecraft, "access_token")
        report["stages"].append({"stage": stage, "status": "succeeded"})
        stage = "entitlements"
        entitlements = send(ENTITLEMENTS, bearer=minecraft_token)
        items = entitlements.get("items")
        if not isinstance(items, list):
            raise ProbeFailure("invalid_response")
        if not any(isinstance(item, dict) and isinstance(item.get("name"), str)
                   and item["name"] in {"game_minecraft", "product_minecraft"} for item in items):
            raise ProbeFailure("java_entitlement_not_confirmed")
        report["stages"].append({"stage": stage, "status": "succeeded"})
        stage = "minecraft_profile"
        profile = send(PROFILE, bearer=minecraft_token)
        if not re.fullmatch(r"[a-fA-F0-9]{32}", token_string(profile, "id")):
            raise ProbeFailure("invalid_response")
        token_string(profile, "name")
        report["stages"].append({"stage": stage, "status": "succeeded"})
        report["status"] = "succeeded"
    except ProbeFailure as error:
        report["stages"].append({"stage": stage, "status": "failed", "error": error.code,
                                 "httpStatus": error.http_status})
        report["status"] = "failed"


def callback_handler(auth: Authorization, outcome: dict):
    class Callback(BaseHTTPRequestHandler):
        def log_message(self, *_):
            pass  # Default HTTP logging would expose the authorization query.

        def do_GET(self):
            host_matches = self.headers.get("Host", "") == urlsplit(auth.redirect_uri).netloc
            if self.path == "/complete" and host_matches and "result" in outcome:
                body = b"<!doctype html><meta charset=utf-8><title>HustMCL</title>HustMCL received the sign-in response. Return to Codex."
                self.send_response(200)
                self.send_header("Content-Type", "text/html; charset=utf-8")
                self.send_header("Cache-Control", "no-store")
                self.send_header("Referrer-Policy", "no-referrer")
                self.send_header("Content-Security-Policy", "default-src 'none'")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)
                outcome["complete"] = True
                return
            try:
                if "result" in outcome:
                    raise ProbeFailure("callback_already_received")
                outcome["result"] = callback_result(self.path, self.headers.get("Host", ""), auth)
            except (ProbeFailure, ValueError):
                self.send_response(400)
                self.end_headers()
                return
            # Keep one-shot credentials out of the final visible URL and page snapshots.
            self.send_response(303)
            self.send_header("Location", "/complete")
            self.send_header("Cache-Control", "no-store")
            self.send_header("Referrer-Policy", "no-referrer")
            self.send_header("Content-Length", "0")
            self.end_headers()
    return Callback


def run(args) -> int:
    client_id = str(uuid.UUID(args.client_id))
    with LoopbackServer(("127.0.0.1", 0), BaseHTTPRequestHandler) as server:
        auth = Authorization(client_id, f"http://localhost:{server.server_port}",
                             secrets.token_urlsafe(32), secrets.token_urlsafe(48))
        outcome = {}
        server.RequestHandlerClass = callback_handler(auth, outcome)
        server.timeout = 0.5
        args.request.parent.mkdir(parents=True, exist_ok=True)
        args.request.write_text(json.dumps({"authorizationUrl": auth.url(), "redirectUri": auth.redirect_uri,
                                           "pid": os.getpid()}, indent=2), encoding="utf-8")
        print("AUTH_REQUEST_READY", flush=True)
        deadline = time.monotonic() + args.timeout
        while not outcome.get("complete") and time.monotonic() < deadline:
            server.handle_request()
            if "result" in outcome:
                deadline = min(deadline, time.monotonic() + 3)
    report = {"schemaVersion": 1, "clientId": client_id, "flow": "authorization_code_pkce",
              "refreshTested": False, "stages": [], "status": "failed"}
    result = outcome.get("result")
    if result is None:
        report["stages"].append({"stage": "browser", "status": "failed", "error": "timeout"})
    elif result[0] == "error":
        report["stages"].append({"stage": "browser", "status": "failed", "error": result[1]})
    else:
        verify_chain(auth, result[1], report)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(json.dumps(report), flush=True)
    return 0 if report["status"] == "succeeded" else 2


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client-id", required=True)
    parser.add_argument("--request", type=Path, default=Path("out/auth-probe/request.json"))
    parser.add_argument("--report", type=Path, default=Path("out/auth-probe/result.json"))
    parser.add_argument("--timeout", type=int, default=600, choices=range(30, 1801), metavar="30..1800")
    args = parser.parse_args()
    try:
        return run(args)
    except Exception:
        # A developer probe must never render an unexpected exception with OAuth context.
        print("AUTH_PROBE_LOCAL_FAILURE", flush=True)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
