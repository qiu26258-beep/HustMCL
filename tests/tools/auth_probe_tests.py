"""Offline behavior checks for the one-shot auth experiment, with no account or network."""
from pathlib import Path
import importlib.util
import json
import io
import sys
import unittest
from urllib.parse import parse_qs, urlencode, urlsplit

spec = importlib.util.spec_from_file_location("hustmcl_auth_probe", Path(__file__).resolve().parents[2] / "tools/verify_minecraft_auth.py")
probe = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = probe
spec.loader.exec_module(probe)


class AuthProbeTests(unittest.TestCase):
    def setUp(self):
        self.auth = probe.Authorization("00000000-0000-0000-0000-000000000001", "http://localhost:12345",
                                        "test-state", "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk")

    def test_pkce_uses_rfc_vector_and_only_signin_scope(self):
        url = urlsplit(self.auth.url())
        self.assertEqual("https://login.microsoftonline.com", f"{url.scheme}://{url.netloc}")
        params = parse_qs(url.query)
        self.assertEqual(["E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM"], params["code_challenge"])
        self.assertEqual(["S256"], params["code_challenge_method"])
        self.assertEqual(["XboxLive.signin"], params["scope"])
        self.assertNotIn(self.auth.verifier, self.auth.url())
        self.assertNotIn(self.auth.verifier, repr(self.auth))
        self.assertNotIn(self.auth.state, repr(self.auth))

    def test_callback_accepts_matching_state_and_code(self):
        self.assertEqual(("code", "one-use-code"), probe.callback_result(
            "/?" + urlencode({"state": "test-state", "code": "one-use-code"}), "localhost:12345", self.auth))

    def test_callback_rejects_wrong_host_state_path_duplicates_and_ambiguity(self):
        cases = [
            ("/?state=wrong&code=secret", "localhost:12345"),
            ("/?state=%E4%B8%AD%E6%96%87&code=secret", "localhost:12345"),
            ("/?state=test-state&code=secret", "evil.example"),
            ("/favicon.ico?state=test-state&code=secret", "localhost:12345"),
            ("http://evil.example/?state=test-state&code=secret", "localhost:12345"),
            ("/?state=test-state&state=test-state&code=secret", "localhost:12345"),
            ("/?state=test-state&code=a&code=b", "localhost:12345"),
            ("/?state=test-state&code=secret&error=access_denied", "localhost:12345"),
            ("/?state=test-state&code=", "localhost:12345"),
        ]
        for path, host in cases:
            with self.subTest(path=path), self.assertRaises(probe.ProbeFailure) as raised:
                probe.callback_result(path, host, self.auth)
            self.assertNotIn("secret", str(raised.exception))

    def test_user_denial_does_not_echo_remote_description(self):
        self.assertEqual(("error", "access_denied"), probe.callback_result(
            "/?state=test-state&error=access_denied&error_description=private", "localhost:12345", self.auth))

    def test_http_callback_redirects_to_a_clean_url_without_echoing_code(self):
        class InMemorySocket:
            def __init__(self, path):
                self.input = io.BytesIO(f"GET {path} HTTP/1.0\r\nHost: localhost:12345\r\n\r\n".encode())
                self.output = bytearray()

            def makefile(self, *_):
                return self.input

            def sendall(self, data):
                self.output.extend(data)

        outcome = {}
        handler = probe.callback_handler(self.auth, outcome)
        first = InMemorySocket("/?state=test-state&code=private-one-use-code")
        handler(first, ("127.0.0.1", 12345), None)
        self.assertIn(b"303", first.output)
        self.assertIn(b"Location: /complete", first.output)
        self.assertNotIn(b"private-one-use-code", first.output)
        final = InMemorySocket("/complete")
        handler(final, ("127.0.0.1", 12345), None)
        self.assertIn(b"200", final.output)
        self.assertNotIn(b"private-one-use-code", final.output)
        self.assertTrue(outcome["complete"])

    def test_error_mapping_is_bounded_and_drops_response_values(self):
        for data in ({"error": {"private": "token"}}, {"error": "private-token"}, {"message": "private-token"}, None):
            self.assertEqual("remote_error", probe.remote_error(data))
        self.assertEqual("aadsts_50011", probe.remote_error({"error_codes": [50011], "error_description": "private"}))
        self.assertEqual("invalid_app_registration", probe.remote_error({"errorMessage": "Invalid app registration"}))

    def test_requests_refuse_unknown_destinations_and_redirects(self):
        with self.assertRaises(probe.ProbeFailure):
            probe.request_json("https://evil.example/", bearer="private")
        with self.assertRaises(probe.ProbeFailure):
            probe.NoRedirect().redirect_request(None, None, 302, "", {}, "https://evil.example/")

    def fake_send(self, failed=None, mismatched_hash=False):
        calls = []
        responses = {
            probe.TOKEN: {"access_token": "private-microsoft-token"},
            probe.USER: {"Token": "private-xbox-token", "DisplayClaims": {"xui": [{"uhs": "12345"}]}},
            probe.XSTS: {"Token": "private-xsts-token", "DisplayClaims": {"xui": [{"uhs": "99999" if mismatched_hash else "12345"}]}},
            probe.MINECRAFT: {"access_token": "private-minecraft-token"},
            probe.ENTITLEMENTS: {"items": [{"name": "game_minecraft"}]},
            probe.PROFILE: {"id": "a" * 32, "name": "PrivatePlayer"},
        }

        def send(url, payload=None, bearer=None):
            calls.append(url)
            if url == failed:
                raise probe.ProbeFailure("remote_error", 403)
            return responses[url]
        return send, calls

    def test_each_failure_stops_before_later_services(self):
        order = [probe.TOKEN, probe.USER, probe.XSTS, probe.MINECRAFT, probe.ENTITLEMENTS, probe.PROFILE]
        for index, endpoint in enumerate(order):
            report = {"stages": []}
            send, calls = self.fake_send(failed=endpoint)
            probe.verify_chain(self.auth, "private-code", report, send)
            self.assertEqual(order[:index + 1], calls)
            self.assertEqual("failed", report["status"])
            self.assertNotIn("private", json.dumps(report))

    def test_success_report_contains_no_credentials_or_player_identity(self):
        report = {"stages": []}
        send, calls = self.fake_send()
        probe.verify_chain(self.auth, "private-code", report, send)
        self.assertEqual("succeeded", report["status"])
        self.assertEqual(6, len(calls))
        for secret in ("private", "PrivatePlayer", "12345", "a" * 32, self.auth.verifier):
            self.assertNotIn(secret, json.dumps(report))

    def test_changed_xbox_identity_never_reaches_minecraft(self):
        report = {"stages": []}
        send, calls = self.fake_send(mismatched_hash=True)
        probe.verify_chain(self.auth, "private-code", report, send)
        self.assertEqual("user_hash_changed", report["stages"][-1]["error"])
        self.assertNotIn(probe.MINECRAFT, calls)


if __name__ == "__main__":
    unittest.main()
