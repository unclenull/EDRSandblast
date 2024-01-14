import os
import sys
import re
import requests
import zlib
import json

if len(sys.argv) < 3:
    print("CLI filename folder")
    exit()
filename = sys.argv[1]
folder = sys.argv[2]

downloadVersion = None
if len(sys.argv) == 4:
    downloadVersion = sys.argv[3]

def download_file(url, local_filename):
    with requests.get(url, stream=True) as r:
        r.raise_for_status()
        total_size = int(r.headers.get("content-length", 0))
        if total_size == 0:
            print("Total size is 0")
            return
        print(f"Total size is {total_size}")
        block_size = max(10240, round(total_size/10))
        num_blocks = total_size // block_size

        decoder = None
        if not r.headers.get("content-encoding") and r.headers.get("content-type") == 'application/gzip':
            decoder = zlib.decompressobj(16 + zlib.MAX_WBITS)
        with open(local_filename, "wb") as f:
            for i, block in enumerate(r.iter_content(block_size)):
                f.write(decoder.decompress(block) if decoder else block)
                percent_complete = (i * block_size + len(block))  / total_size
                print(f"Progress: {percent_complete*100:.2f}%")


infoFile = os.path.join(folder, filename + ".info.json")
infos = None

if not os.path.isfile(infoFile):
    print(f'Parsed info file does not exist: {infoFile}')
    infoFileRaw = os.path.join(folder, filename + ".info.raw.json")
    if not os.path.isfile(infoFileRaw):
        print(f'Raw info file does not exist: {infoFileRaw}')
        download_file(
            "https://winbindex.m417z.com/data/by_filename_compressed/"
            + filename
            + ".json.gz",
            infoFileRaw,
        )

    with open(infoFileRaw, "r", encoding="utf-8") as f:
        infosRaw = json.load(f)

    infos = {}
    for item in infosRaw.values():
        if 'fileInfo' in item:
            fileInfo = item['fileInfo']
            if 'version' in fileInfo:
                version = fileInfo['version'].split(' ')[0]
            else:
                # import pdb; pdb.set_trace()
                obj = next(iter(item["windowsVersions"].values()))
                obj = next(iter(obj.values()))
                if 'assemblies' not in obj:
                    continue
                obj = next(iter(obj['assemblies'].values()))
                version = obj['assemblyIdentity']['version']
            sha256 = ''
            if 'sha256' in fileInfo:
                sha256 = fileInfo['sha256']
            infos[version] = { 'sha256': sha256, 'timestamp': fileInfo['timestamp'], 'virtualSize': fileInfo['virtualSize'] }

    infos = dict(sorted(infos.items()))
    f = open(infoFile, "w", encoding="utf-8")
    json.dump(infos, f, indent=2)
    f.close()

if not infos:
    with open(infoFile, "r", encoding="utf-8") as f:
        infos = json.load(f)

downloadInfos = {}
if downloadVersion:
    info = infos[downloadVersion]
    if not info:
        print(f'No info for the version ${downloadVersion}')
        exit()
    downloadInfos[downloadVersion] = info
    info['version'] = downloadVersion
else:
    # Get the first one in each major version
    p = re.compile(r'(\d+)\.(\d+).(\d+).(\d+)')
    for k in infos.keys():
        m = p.match(k)
        if not m:
            print(f'Problem with the version {k}')
            continue
        major = m.group(3)
        if major not in downloadInfos:
            downloadInfos[major] = infos[k]
            downloadInfos[major]['version'] = k

print(f'Total files to download: {len(downloadInfos)}')
for i, info in enumerate(downloadInfos.values()):
    parts = filename.split('.')
    binFile = parts[0] + '.' + info['version'] + '.' + parts[1]
    binFile = os.path.join(folder, binFile)
    if os.path.isfile(binFile):
        print(f'File already exists: {binFile}')
        continue
    fileId = ('0000000' + hex(info['timestamp']))[-8:].upper() + hex(info['virtualSize'])[2:]
    print(f'Downloading file {i}: {binFile}')
    download_file('https://msdl.microsoft.com/download/symbols/' + filename + '/' + fileId + '/' + filename, binFile)
