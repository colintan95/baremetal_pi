from datetime import datetime
from pathlib import Path
import subprocess
import re

SRC_DIR = "src"
OUT_DIR = "out"

def is_src(path):
  return path.is_file() and (path.suffix == '.c' or path.suffix == '.S')

def get_out_path(src):
  return Path(OUT_DIR) / src.with_suffix('.o').name

srcs = [p for p in Path(SRC_DIR).iterdir() if is_src(p)]

src_infos = [{'src': p, 'out': get_out_path(p)} for p in srcs]

def is_out_of_date(src, out):
  result = subprocess.run(['clang', '-M', src], capture_output=True, text=True)
  match = re.search(r'(.+\.o):(.*)', result.stdout)

  # TODO: Make sure the out file from clang matches ours

  deps = match.group(2).split()
  deps = [Path(s) for s in deps if s != '\\']

  out_mod_time = out.stat().st_mtime

  for dep in deps:
    mod_time = dep.stat().st_mtime
    if mod_time > out_mod_time:
      return True

  return False

has_rebuild = False

for info in src_infos:
  src = info['src']
  out = info['out']

  if (is_out_of_date(src, out)):
    cmd = f'clang --target=aarch64-elf -O1 -c {src} -o {out}'

    print(cmd)
    subprocess.run(cmd, shell=True)

    has_rebuild = True

target = 'out/kernel8.img'

if has_rebuild:
  outs = [info['out'] for info in src_infos]
  args = ' '.join([str(p) for p in outs])

  cmds = [f'ld.lld -m aarch64elf {args} -T link.ld -o out/kernel8.elf',
      f'llvm-objcopy -O binary out/kernel8.elf {target}']

  for cmd in cmds:
    print(cmd)
    subprocess.run(cmd, shell=True)

else:
  print(f'\'{target}\' up to date')
