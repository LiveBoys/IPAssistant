# SignPath integration

This directory documents the source-controlled configuration for the IP
Assistant application to apply for the SignPath Foundation open-source code
signing service.

## Code signing policy

Free code signing is provided by [SignPath.io](https://signpath.io/); the
certificate is provided by
[SignPath Foundation](https://signpath.org/).

- Project: IP Assistant
- License: MIT
- Maintainer, reviewer, and signing approver: Jiugang He
- Source repository: https://github.com/LiveBoys/IPAssistant
- Privacy policy:
  https://github.com/LiveBoys/IPAssistant/blob/main/PRIVACY.md
- Build origin: GitHub-hosted Windows runners only
- Release trigger: an annotated or lightweight tag matching `v*`
- Approval: every production signing request requires SignPath approval

Only artifacts built from the public tagged source and the checked-in release
workflow are eligible for release signing. Third-party Qt runtime libraries
are distributed as upstream components and are not signed with the IP
Assistant project certificate.

IP Assistant does not transfer information to other networked systems unless
the user explicitly requests a network configuration change or uses a
third-party website to obtain the software.

## Signing sequence

The release workflow uses two signing requests:

1. `release-binaries` signs `IPAssistant.exe` and
   `IPAssistantService.exe`.
2. The signed binaries are placed into the Inno Setup payload.
3. `release-installer` signs the completed
   `IPAssistantSetup-<version>.exe`.

This order ensures that both the installed project binaries and the outer
installer carry valid Authenticode signatures.

## SignPath project setup

After the SignPath Foundation application is approved:

1. Create or select the SignPath project slug `IPAssistant`.
2. Create the signing policy slug `release-signing`.
3. Create artifact configurations:
   - `release-binaries`, using `artifact-configuration-binaries.xml`;
   - `release-installer`, using `artifact-configuration-installer.xml`.
4. Link the project to the predefined GitHub.com trusted build system.
5. Install the SignPath GitHub App for the public GitHub repository.
6. Add the GitHub repository variable:
   - `SIGNPATH_ORGANIZATION_ID`
7. Add the GitHub Actions secret:
   - `SIGNPATH_API_TOKEN`

The release workflow refuses to publish a tagged GitHub Release when these
values are absent. Manual workflow runs may still produce unsigned artifacts
for testing, but those artifacts must not be presented as official releases.

## Verification

Verify signed files on Windows:

```powershell
Get-AuthenticodeSignature .\IPAssistant.exe
Get-AuthenticodeSignature .\IPAssistantService.exe
Get-AuthenticodeSignature .\IPAssistantSetup-1.0.0.exe

signtool verify /pa /all /v .\IPAssistant.exe
signtool verify /pa /all /v .\IPAssistantService.exe
signtool verify /pa /all /v .\IPAssistantSetup-1.0.0.exe
```

Each result must report a valid signature and a trusted certificate chain.
