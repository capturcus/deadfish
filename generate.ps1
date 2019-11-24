flatc.exe --ts  --no-fb-import -o .\client\src .\deadfish.fbs
flatc.exe --rust -o .\server\src .\deadfish.fbs

@("import { flatbuffers } from 'flatbuffers';") +  (Get-Content ".\client\src\deadfish_generated.ts") | Set-Content ".\client\src\deadfish_generated.ts"