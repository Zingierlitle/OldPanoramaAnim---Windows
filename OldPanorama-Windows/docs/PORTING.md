# Target verification

Upstream revision: `97756adf943ee20bddf42d35b6958e09cec19af8`.
The original uses Android ELF APIs, RTTI, vtables and instruction signatures.
This implementation uses Windows PE metadata and an exact executable hash.

Static-analysis chain for the supported executable:

1. `options.screenAnimations` at RVA `0x8b89e20` is referenced by `0x1d0af00`.
2. Registration caller `0x1cc8710` uses option ID `0x1e8`.
3. Getter `0x1cb5f10` reads that option.
4. Vtables `0x88b20a8` and `0x88b2e28` reference it at offset `0x3d0`.
5. Render function `0x2062990` calls that slot at `0x206301a`.
6. Jump `0x206302c` skips a double timer update at object offset `0x4a0`.
7. The timer is read at `0x2063129` and passed to rotation-matrix math.
8. Only the branch bytes `74 1e` become `90 90`.

This mapping is based on static analysis; actual panorama behavior still needs
in-game validation. The general option getter and splash text are not patched.

Before supporting another build, independently trace its option and rendering
code, verify the branch, and add its exact hash and instruction window. Do not
weaken the guard or guess offsets. Test panorama motion with animations off,
ordinary rendering, and unchanged menu transitions.

Self-tests check mismatched bytes, two-byte-only changes, restored executable-page
protection, repeat-patch rejection and wrong-host rejection. The build also loads
the real DLL into a separate PowerShell process and checks its diagnostic log.
These tests do not simulate Minecraft rendering.

## Broad version support

The manifest deliberately omits game-version restrictions. Actual additional
build support requires either individually verified targets or a rigorously
validated semantic scanner. This release uses fixed RVAs and one verified
executable hash. Removing its runtime checks would make the same offsets apply
to unrelated code on other builds; it would not provide compatibility.
No additional Minecraft builds have been validated or added in this update.
