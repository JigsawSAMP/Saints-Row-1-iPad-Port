from pathlib import Path
import subprocess,shlex
root=Path(__file__).resolve().parents[2]
build=root/'PortBuild/ipad-port'
commands=subprocess.check_output(['ninja','-C',str(build),'-t','commands'],text=True)
command=next(c for c in commands.splitlines() if ' -c ' in c and 'AppleCoop.cpp.o -c' in c)
tokens=shlex.split(command); includes=[]; i=0
while i<len(tokens):
    if tokens[i].startswith('-I'): includes.append(tokens[i])
    elif tokens[i]=='-isystem': includes+=tokens[i:i+2]; i+=1
    i+=1
binary=root/'PortBuild/tools/coop_transport_test'
subprocess.run(['/usr/bin/clang++','-std=c++23','-O1','-g','-fsanitize=address','-fno-omit-frame-pointer','-Wno-format-truncation',*includes,str(build/'coop/AppleCoop.cpp'),str(root/'PortSupport/tests/coop_transport.cpp'),'-o',str(binary)],check=True)
subprocess.run([str(binary),str(root/'PortBuild/coop-transport-test')],check=True)
