# Security Policy

This policy describes how to report security vulnerabilities in products from Utilitech AS (amsleser.no), and how we handle such reports. It is our coordinated vulnerability disclosure (CVD) policy as required by the EU Cyber Resilience Act (Regulation (EU) 2024/2847).

## Scope

This policy covers:

- The amsreader firmware in this repository
- Utilitech hardware products running this firmware: Pow-K, Pow-U and Pow-P1 HAN port readers

Not covered: third-party forks or modified builds of the firmware, and third-party hardware not sold by Utilitech AS.

## Supported versions

Security fixes are provided in new releases of the firmware. We recommend always running the latest stable release.

| Version | Supported |
| --- | --- |
| Latest stable release | Yes |
| Release candidates (-rc) | Fixes are included in the next release |
| Older releases | No; please upgrade |

## Reporting a vulnerability

**Please do not report security vulnerabilities as public GitHub issues.**

Report vulnerabilities privately through GitHub:

1. Go to the **Security** tab of this repository
2. Click **Report a vulnerability**

Direct link: https://github.com/UtilitechAS/amsreader-firmware/security/advisories/new

If you cannot use GitHub, you may instead send an e-mail to post (at) amsleser.no with the subject "Security vulnerability".

Please include as much as possible of the following:

- Affected product(s) and firmware version
- Description of the vulnerability and its potential impact
- Steps to reproduce, or a proof of concept
- Whether the vulnerability is known to be actively exploited
- Whether you wish to be credited, and under which name

Reports may be written in English or Norwegian.

## What happens next

- We acknowledge receipt within 5 working days.
- We assess the report and inform you whether we consider it a vulnerability, normally within 14 days.
- We develop and release a fix without undue delay, and keep you informed of progress.
- When a fix is available, we publish a security advisory in this repository describing the vulnerability and the fixed version. We credit the reporter unless they prefer to remain anonymous.

## Coordinated disclosure

We ask that you do not disclose the vulnerability publicly until a fix has been released, or until 90 days have passed since your report, whichever comes first. If a fix requires more time, we will agree on a new date with you.

We will not take legal action against anyone who reports a vulnerability in good faith and in accordance with this policy.

## Non-security issues

Ordinary bugs and feature requests are reported as public issues: https://github.com/UtilitechAS/amsreader-firmware/issues
