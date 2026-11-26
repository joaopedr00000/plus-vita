"""Exercise the actual normalized VPK reader with host fixtures."""
import pathlib, struct, subprocess, sys, tempfile, zipfile
exe=sys.argv[1]
def sfo(title):
    key=b"TITLE_ID\0";value=title.encode()+b"\0"
    return struct.pack("<4sIIII",b"\0PSF",0x101,36,36+len(key),1)+struct.pack("<HHIII",0,0x204,len(value),len(value),0)+key+value
with tempfile.TemporaryDirectory(prefix="pv-update-") as td:
    root=pathlib.Path(td)
    for j,(title,extra,want) in enumerate([
        ("PLVTB0001",None,"ok"),("OTHER0001",None,"bad"),
        ("PLVTB0001",("sce_sys/../escape","bad"),"bad"),
        ("PLVTB0001",("ux0:app/eboot.bin","bad"),"bad"),
        ("PLVTB0001",("sce_sys//escape","bad"),"bad"),
    ]):
        path=root/f"fixture{j}.vpk"
        with zipfile.ZipFile(path,"w",compression=zipfile.ZIP_DEFLATED) as z:
            z.writestr("eboot.bin",b"SCE\0"+b"x"*4096)
            z.writestr("sce_sys/param.sfo",sfo(title))
            z.writestr("licenses/test.txt",b"test")
            if extra:z.writestr(*extra)
        subprocess.run([exe,str(path),str(root/f"UpdateStage-{j}"),want],check=True)
        assert not (root/"escape").exists()
    path=root/"stored.vpk"
    with zipfile.ZipFile(path,"w",compression=zipfile.ZIP_STORED) as z:
        z.writestr("eboot.bin",b"SCE\0"+b"x"*4096)
        z.writestr("sce_sys/param.sfo",sfo("PLVTB0001"))
    subprocess.run([exe,str(path),str(root/"UpdateStage-99"),"ok"],check=True)
print("Stored and deflated packages; rejected foreign title and traversal")
