# Fork and upload

1. Sign in and open https://github.com/CrackedMatter/mcpelauncher-old-screen-animations/fork
2. Create the fork under your account. No fork has been created by this local project.
3. Create a branch named `windows-levilauncher` in your fork.
4. Use **Add file > Upload files** to upload this folder's contents at repository root.
5. Retain the upstream LICENSE and Android source. Windows uses `src/main.c`,
   while upstream uses `src/main.cpp`; both can coexist.
6. Include hidden files: `.github/workflows/build.yml`, `.gitignore`, and `.gitattributes`.
7. Open Actions, enable workflows if prompted, and run **Build Windows DLL**.
8. Download the artifact. Publish the generated ZIP through a Release when ready.

Upload source contents, not an enclosing OldPanorama-Windows directory.
Do not upload Minecraft, resource packs, local toolchains, build output or logs.
The source ZIP beside this folder includes only an explicit project file list.
