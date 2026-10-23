import copy,io,json,pathlib,subprocess,sys,tempfile,zipfile

def run(executable):
    with tempfile.TemporaryDirectory() as temp:
        root=pathlib.Path(temp)
        manifest={'formatVersion':1,'game':'minecraft','name':'测试 Pack','versionId':'1','dependencies':{'minecraft':'1.21.1'},'files':[]}
        def data(m=manifest): return json.dumps(m,ensure_ascii=False).encode()
        def write(name,items):
            with zipfile.ZipFile(root/name,'w',compression=zipfile.ZIP_DEFLATED) as z:
                for k,v in items:
                    if isinstance(k,str) and '\\' in k:
                        # ZipInfo normalizes Windows separators at construction; preserve hostile wire bytes.
                        info=zipfile.ZipInfo(k);info.filename=k
                        z.writestr(info,v)
                    else:z.writestr(k,v)
        write('allow-direct.zip',[('modrinth.index.json',data())])
        write('allow-wrapped.zip',[('folder/modrinth.index.json',data())])
        inner=io.BytesIO()
        with zipfile.ZipFile(inner,'w',compression=zipfile.ZIP_DEFLATED) as z:z.writestr('modrinth.index.json',data())
        write('allow-inner.zip',[('modpack.mrpack',inner.getvalue()),('Launcher.exe',b'not-executed')])
        write('allow-overrides.zip',[('modrinth.index.json',data()),('client-overrides/config/x.txt',b'client'),('overrides/config/x.txt',b'common')])
        server=copy.deepcopy(manifest);server['files']=[{'path':'mods/server.jar','env':{'client':'unsupported','server':'required'}}]
        write('allow-server-only.zip',[('modrinth.index.json',data(server))])
        for i,path in enumerate(['../escape','/absolute','C:/escape','a\\b','CON.jar','mods/file:stream','mods/x.']):
            write(f'reject-path-{i}.zip',[('modrinth.index.json',data()),(path,b'x')])
        write('reject-case.zip',[('modrinth.index.json',data()),('overrides/A.txt',b'a'),('overrides/a.txt',b'b')])
        write('reject-directory.zip',[('modrinth.index.json',data()),('overrides/config',b'a'),('overrides/config/x',b'b')])
        write('reject-layer-directory.zip',[('modrinth.index.json',data()),('overrides/config',b'a'),('client-overrides/config/x',b'b')])
        write('reject-ambiguous.zip',[('modrinth.index.json',data()),('other/modrinth.index.json',data())])
        write('reject-two-inner.zip',[('modpack.mrpack',inner.getvalue()),('modpack.zip',inner.getvalue())])
        write('reject-other-format.zip',[('manifest.json',b'{}')])
        write('reject-bad-json.zip',[('modrinth.index.json',b'{bad')])
        bad=copy.deepcopy(manifest);bad['dependencies']['unknown']='1'
        write('reject-dependency.zip',[('modrinth.index.json',data(bad))])
        bad=copy.deepcopy(manifest);bad['files']=[{'path':'mods/a.jar','hashes':{'sha1':'a'*40,'sha512':'b'*128},'fileSize':1,'downloads':['https://cdn.modrinth.com.evil.test/x']}]
        write('reject-host.zip',[('modrinth.index.json',data(bad))])
        link=zipfile.ZipInfo('overrides/link');link.create_system=3;link.external_attr=(0o120777<<16)
        write('reject-link.zip',[('modrinth.index.json',data()),(link,b'outside')])
        (root/'reject-corrupt.zip').write_bytes(b'not a zip')
        result=subprocess.call([executable,str(root)])
        if result: print('archive test process exit:',result,flush=True)
        return result
if __name__=='__main__':sys.exit(run(sys.argv[1]))
