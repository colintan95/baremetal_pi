from pathlib import Path
import subprocess
import re

def is_out_of_date(src, out):
  result = subprocess.run(['clang', '-M', src], capture_output=True, text=True)

  # The dep list can be on multiple lines so use the dot all flag to include
  # newlines
  match = re.search(r'(.+\.o):(.*)', result.stdout, flags=re.DOTALL)

  # TODO: Make sure the out file from clang matches ours

  deps = match.group(2).split()
  deps = [Path(s) for s in deps if s != '\\']

  out_mod_time = out.stat().st_mtime

  for dep in deps:
    mod_time = dep.stat().st_mtime
    if mod_time > out_mod_time:
      return True

  return False

TARGET = 'out/kernel8.img'

SRC_DIR = "src"
OUT_DIR = "out"

def main():
  src_dir = Path(SRC_DIR)
  out_dir = Path(OUT_DIR)

  def is_src(path):
    return path.is_file() and (path.suffix == '.c' or path.suffix == '.S')

  def get_out_path(src):
    return out_dir / src.with_suffix('.o').name

  srcs = [p for p in src_dir.iterdir() if is_src(p)]
  src_infos = [{'src': p, 'out': get_out_path(p)} for p in srcs]

  has_rebuild = False

  for info in src_infos:
    src = info['src']
    out = info['out']

    if (is_out_of_date(src, out)):
      cmd = f'clang --target=aarch64-elf -O1 -c {src} -o {out}'

      print(cmd)
      subprocess.run(cmd, shell=True)

      has_rebuild = True

  target = Path(TARGET)

  if has_rebuild:
    outs = [info['out'] for info in src_infos]
    args = ' '.join([str(p) for p in outs])

    elf = target.with_suffix('.elf')

    cmds = [f'ld.lld -m aarch64elf {args} -T link.ld -o {elf}',
        f'llvm-objcopy -O binary {elf} {target}']

    for cmd in cmds:
      print(cmd)
      subprocess.run(cmd, shell=True)

  else:
    print(f'\'{target}\' up to date')

if __name__ == '__main__':
  main()
