"""Generate an Apple transport adaptation; retain upstream protocol and game logic."""
from pathlib import Path
import re, sys
root=Path(__file__).resolve().parent.parent
out=Path(sys.argv[1]); out.mkdir(parents=True,exist_ok=True)
s=(root/'SaintsReborn/core/WhompaysCoop/coop.cpp').read_text()
a=s.index('#define WIN32_LEAN_AND_MEAN'); b=s.index('#include <algorithm>')
s=s[:a]+'#include "AppleCoopPlatform.h"\n'+s[b:]
s=s.replace('#pragma comment(lib, "ws2_32.lib")','')
s=s.replace('  probe_log << MonoMs() << " " << message << std::endl;', '''  if(probe_log.tellp()>=std::streampos(1024*1024)) {
    probe_log.close();
    probe_log.open(std::string(self->folder)+"/coop-"+std::to_string(GetCurrentProcessId())+".log",std::ios::trunc);
  }
  probe_log << MonoMs() << " " << message << std::endl;''')
# All Windows path literals in this source denote files, not escape characters.
s=s.replace('\\\\','/')
a=s.index('LPTOP_LEVEL_EXCEPTION_FILTER previous_crash_filter'); b=s.index('// Streaming groups are set up',a)
s=s[:a]+'''void StartCrashReports(const char* folder) { mod_folder=folder; }
// iPadOS crash diagnostics are collected by the operating system.
'''+s[b:]
a=s.index('    DWORD pid=0; HWND fg=GetForegroundWindow();'); b=s.index('\n  }',a)
s=s[:a]+'    f6_down=api->key_down(VK_F6)!=0;'+s[b:]
s=s.replace('F6 open  |  1 host  2 join  3 stop  4 host online  5 copy of you', 'Ready. Use Settings / Co-op to host or join by IP.')
s=s.replace('int from_len = sizeof(from);','socklen_t from_len = sizeof(from);')
a=s.index('std::string LocalIp() {'); b=s.index('\nvoid CloseSocket()',a)
s=s[:a]+'std::string LocalIp() { return SRCoopLocalIp(); }\n'+s[b:]
s+='\nextern \"C\" const char* SRAppleCoopStatus() { thread_local std::string copy; copy=GetStatus(); return copy.c_str(); }\n'
(out/'AppleCoop.cpp').write_text(s)
# Static, signed AOT entry points replace the Windows MinHook patches.
addresses=sorted(set(re.findall(r'api->hook\(0x([0-9A-Fa-f]{8})',s)+re.findall(r'Trace<0x([0-9A-Fa-f]{8})>',s)))
for address in addresses:
    stubs=(root/'SaintsReborn/project/src/stubs.cpp').read_text()
    if re.search(r'PPC_FUNC\(sub_'+address+r'\)',stubs): raise RuntimeError('Existing hook collision: '+address)
(out/'CoopHooks.inc').write_text(''.join('SR_COOP_HOOK('+a+')\n' for a in addresses))
print('Prepared Apple co-op with',len(addresses),'static hooks')
