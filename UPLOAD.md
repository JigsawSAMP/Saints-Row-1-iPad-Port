# Upload the source to GitHub

1. Extract the source ZIP. Use the contents of its `SaintsRow-iPad` folder as the repository root.
2. Create an empty GitHub repository in your account. Do not initialize another README/license if you plan to push the supplied files.
3. Upload the extracted source files (including `.gitignore`, documentation and `licenses`), or initialize Git locally and push. Do not upload the ZIP alone if you want browsable source.
4. Keep `SaintsReborn`, `PortDependencies`, `PortBuild`, game files, signed apps and credentials excluded. Build instructions fetch upstream repositories at pinned revisions.

The package contains no Git history or private signing identity. Before a Git commit, choose a public/anonymous author name and a GitHub-provided noreply email in your local Git configuration; Git commit metadata is public and is controlled by your upload workflow. Avoid your work/college email or real name if you want anonymity. If you use GitHub's web uploader, GitHub may attribute the upload to your signed-in profile.

Retain the upstream credit and license notices. Do not describe this as a stable/full 60 FPS port or proven PC cross-play. `SOURCE-MANIFEST.sha256` lists the packaged source-file hashes; keep it for archive verification, and regenerate it after changing files.
