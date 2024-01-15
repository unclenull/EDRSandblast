import os
import sys
import glob
import subprocess

if len(sys.argv) < 3:
    print("CLI glob|file func1 func2 ...")
    exit()
target = sys.argv[1]
funcNames = sys.argv[2:]

files = []
if target.find('*') > -1:
    files = list(glob.glob(target))
else:
    files = [target]

def run(args, **kargs):
    """Wrap subprocess.run to works on Windows and Linux"""
    # Windows needs shell to be True, to locate binary automatically
    # On Linux, shell needs to be False to manage lists in args
    shell = sys.platform in ["win32"]
    return subprocess.run(args, shell=shell, **kargs)

print(f'Total files to process: {len(files)}')
idpi = ';'.join(tuple(map(lambda func: f'idpi~{func}$', funcNames)))
for i, f in enumerate(files):
    print(f'Process file {i}: {f}')
    # get address
    try:
        r = run(["r2", "-c", f"e log.level=0; idpd; {idpi}", "-qq", f], check=True, capture_output=True, text=True)
    except Exception as e:
        print(f"An error occurred: {e}")
    lines = list(r.stdout.splitlines())[1:]
    addrs = tuple(map(lambda l: l.split(' ')[0], lines))
    print(f'address list are {addrs}')
    # disassemble
    out_file = f + '.asm'
    pdf = ';'.join(tuple(map(lambda addr: f's{addr};af;pdf>>{out_file}', addrs)))

    try:
        r = run(["r2", "-c", f"e log.level=1;e asm.offset.relative=true;{pdf}", "-qq", f], check=True, capture_output=True, text=True)
        if r.stderr.startswith('ERROR:'):
            print('Try with pd')
            pd = ';'.join(tuple(map(lambda addr: f's{addr};af;pd>>{out_file}', addrs)))
            r = run(["r2", "-c", f"e log.level=1;e asm.offset.relative=true;{pd}", "-qq", f])
    except Exception as e:
        print(f"An error occurred: {e}")
