## What this adds

<!-- Device(s) and vendor/product IDs, e.g. "Acme M1 (1234:5678)". -->

## Where the protocol comes from

<!-- Link the source (another project, a vendor web driver, your own USB capture). -->

## Tested on real hardware?

<!-- Yes (which device, what --list printed) / No -->

## Checklist

- [ ] Only read-only battery queries are sent; no settings, lighting or firmware are changed
- [ ] Only devices recognised by vendor/product ID (or a protocol-specific collection) are touched
- [ ] No network access, no launching other programs, no new files outside `src/`, `docs/` or `README.md`
- [ ] No binaries or generated files are included
- [ ] No `Co-authored-by` lines crediting AI tools (Claude, Copilot, ChatGPT ...) in the commit messages
- [ ] Changes to `.github/`, `installer/`, `build.bat` or `Makefile` are explained above
