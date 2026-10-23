import hashlib,json,pathlib,subprocess,sys,tempfile,zipfile
with tempfile.TemporaryDirectory() as temp:
    root=pathlib.Path(temp)
    mod=b'test mod bytes';config=b'downloaded config'
    files=[]
    for name,body in [('mods/sample.jar',mod),('config/download.txt',config)]:
        files.append({'path':name,'fileSize':len(body),'hashes':{'sha1':hashlib.sha1(body).hexdigest(),'sha512':hashlib.sha512(body).hexdigest()},'downloads':['https://cdn.modrinth.com/test/'+name]})
        (root/hashlib.sha1(body).hexdigest()).write_bytes(body)
    for name,loader in [('pack',None),('forge','forge'),('failed',None)]:
        manifest={'formatVersion':1,'game':'minecraft','name':'测试 '+name,'versionId':'1','dependencies':{'minecraft':'1.21.1'},'files':files}
        if loader:manifest['dependencies'][loader]='52.0.0'
        with zipfile.ZipFile(root/(name+'.zip'),'w',compression=zipfile.ZIP_DEFLATED) as z:
            z.writestr('modrinth.index.json',json.dumps(manifest).encode())
            z.writestr('overrides/config/common.txt',b'base')
            z.writestr('client-overrides/config/common.txt',b'client')
            z.writestr('overrides/config/download.txt',b'override')
    result=subprocess.call([sys.argv[1],str(root),sys.argv[2]])
    if result:print('workspace test process exit:',result,flush=True)
    sys.exit(result)
