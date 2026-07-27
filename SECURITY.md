# Security Policy

## Supported versions

Security fixes are provided for the latest published release. Older releases
may not receive separate patches.

| Version | Supported |
| --- | --- |
| Latest release | Yes |
| Older releases | No |

## Reporting a vulnerability

Please do not disclose a suspected vulnerability in a public issue, discussion,
or pull request.

Use [GitHub Private Vulnerability Reporting](https://github.com/LiveBoys/IPAssistant/security/advisories/new)
after it is enabled for this repository:

1. Open the repository's **Security** page.
2. Select **Advisories**.
3. Select **Report a vulnerability**.

If private vulnerability reporting is not available, contact the repository
owner through the hosting platform's private messaging mechanism and request a
private reporting channel. Do not send exploit details or private network
information in a public message.

Include, when available:

- the affected IP Assistant version and Windows version;
- a concise description of the impact;
- reproduction steps or a minimal proof of concept;
- whether administrator or local service access is required;
- suggested mitigations;
- your preferred name for acknowledgment.

## Response targets

The maintainer aims to:

- acknowledge a report within 7 calendar days;
- provide an initial assessment within 14 calendar days;
- coordinate a disclosure date after a fix is available;
- credit the reporter when requested and appropriate.

These are targets rather than guarantees for this volunteer-maintained
project.

## Scope

Examples of security-sensitive areas include:

- the GUI-to-service named-pipe boundary;
- Windows service installation and control;
- validation of network profile data;
- command construction and execution;
- update, installer, and code-signing integrity;
- unsafe permissions on profile or configuration files.

Configuration mistakes that only produce an expected temporary network outage
are generally not vulnerabilities unless they cross a security boundary or
allow unauthorized changes.

## Release integrity

Official releases are built from tagged source revisions. Release artifacts
publish a SHA-256 checksum. Once the SignPath Foundation application is
approved and configured, Windows executables and installers are submitted
through the documented SignPath signing workflow.

---

## 中文摘要

请勿在公开 Issue、讨论或 Pull Request 中披露漏洞细节。优先使用代码托管平台的
私密漏洞报告功能；如果暂未启用，请先通过平台私信联系仓库所有者，申请私密报告
渠道。报告建议包含受影响版本、Windows 版本、影响、复现步骤和权限要求。
