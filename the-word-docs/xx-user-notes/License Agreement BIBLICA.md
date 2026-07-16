# Biblica Fast-Track Bible License — Analysis & Notes

Source: `Biblica Fast-Track Bible License v1.docx.txt` (downloaded 2026-07-16)

---

## 1. Rights Granted (§1–3)

| Concept | Detail |
|---------|--------|
| **Electronic Rights** | Text display + audio streaming only. No downloadable media to users. No video/AV combos. |
| **Non-Commercial** | App must be **free**: no ads, subscriptions, paywalls, or selling user data. No gifts/premiums tied to donation solicitation. |
| **No AI/ML** | Cannot use CONTENT to produce personalized content via AI/ML. |
| **Original language only** | NVI can only be displayed in Portuguese. |
| **Non-exclusive, non-transferable** | Cannot assign/sublicense to anyone else. |

**Status**: ✅ TheWord is free, open-source, text-only Bible reader — fits well.

---

## 2. Content Integrity (§5.D)

> "The CONTENT shall be used in the form and format provided via SDKs or direct API calls, and ORGANIZATION shall make no alterations to the CONTENT prior to publication of the WORK. All footnotes to the TRANSLATIONS text must be included along with the TRANSLATIONS text and accessible to the end-user."

**Impact:**
- ⚠️ **Current code strips footnotes** — `BibleClient.cpp` calls `StripFootnotes()` which removes `<span class="yv-n">` elements. This **must** be removed.
- ✅ Footnote display is already on the Release Plan for v1.7.0 — this makes it mandatory rather than optional.
- Must ensure all API-provided content passes through unaltered.

---

## 3. Encryption / Security (§5.E)

> "ORGANIZATION shall use appropriate and effective industry standard encryption measures to prevent the unauthorized supply, onward-supply, or reproduction of all editions of the WORK."

**Reference**: https://www.biblica.com/digital-ip-protection-requirements/

**Impact:**
- API transport is already HTTPS/TLS ✅
- Must ensure content is **never stored unencrypted on disk**
- API key must not be exposed (currently compiled into binary — acceptable for mobile?)
- **No raw ChapterData caching to disk** — in-memory layout caching should be fine, but persistent caching of API responses would violate this clause

**My notes:**
> Need to audit what gets persisted. Currently we cache layouts (ChapterLayout), not raw text. That's probably OK, but verify.
> API key is in .env / assets — need to ensure protection.

---

## 4. Verse Display Limit (§5.F)

> "ORGANIZATION shall be limited to the number of verses from each text or audio which comprises the CONTENT which may be displayed or streamed per user at any given time to no more than two (2) chapters or twenty-five (25) verses, whichever is greater."

**Interpretation (my understanding):**
- "Whichever is greater" = we can display **2 chapters** even if they total >25 verses.
- Only becomes a restriction for single chapters with <13 verses (where 25 verses > 2 chapters).
- With infinite scrolling, this limits the scroll window to 2 chapters ahead, not the whole book.

**Impact:**
- ✅ Our current "one chapter at a time, scroll to next chapter" model fits perfectly.
- ⚠️ If we ever add book-level continuous scrolling, would need to limit to 2 chapters visible.
- Psalms 119 + 120 = ~200 verses displayed → still OK because it's 2 chapters.

**My notes:**
> Fine for current architecture. If we add continuous reading mode, enforce 2-chapter limit.

---

## 5. Privacy & Data Collection (§5.G–H)

- Content must be **free** to all end users.
- Cannot gather personal info for commercial exploitation.
- Can gather info with **opt-in consent**, but:
  - Cannot distribute to third parties (except YouVersion per SDK License)
  - Must keep secure and confidential
  - Must comply with applicable data protection laws

**Impact:**
- ✅ TheWord doesn't collect user data today.
- ⚠️ If we add any analytics/telemetry in the future, must be opt-in only and never sold.
- GDPR/Brazilian LGPD compliance would apply if collecting data.

---

## 6. Content Approval / Takedown (§5.I)

> "LICENSOR will have the reasonable right to disapprove of any additional content presented with the CONTENT in the WORK. If it is determined by LICENSOR to be inappropriate to the nature of the CONTENT, to the extent that it would bring shame, embarrassment, or disrepute to the CONTENT and/or LICENSOR, or if it is determined to be seriously and demonstrably anti-Christian."

**Process:**
1. Biblica sends written request
2. Organization must remove within **48 hours**
3. Failure → immediate termination of license

**Impact:**
- ⚠️ If TheWord ever includes user notes, commentary, or any non-Bible content, Biblica could theoretically object.
- 48h turnaround for a solo dev is tight but manageable if we have a clear removal process.
- Our content is purely Bible text — unlikely to trigger.

---

## 7. Distributor Approval (§5.J)

Biblica can disapprove of distributors (e.g., app stores) that are "seriously, demonstrably and persistently anti-Christian" as a primary business.

**Impact:**
- Google Play, F-Droid, GitHub Releases → likely fine.
- If we ever distribute through other channels, may need pre-approval.

---

## 8. Prospectus for New Versions (§5.K)

> "Before any new, revised or enhanced product or edition of the WORK may be produced, ORGANIZATION shall submit to LICENSOR a reasonably accurate prospectus describing each new, revised or enhanced edition of the WORK for LICENSOR'S consideration and prior written approval."

**Definition of "enhancement":**
- (i) Perform significantly new or different functions
- (ii) Delivered via a new channel, form, platform or media
- (iii) Combined with other content or media

**My interpretation:**
- This applies to **official releases**, not daily dev commits or alpha builds.
- Initial development and early alpha/beta should not require approval.
- When cutting a new minor/major release, prepare and submit a prospectus.

**Impact:**
- ⚠️ Need a workflow step: on release, draft prospectus → submit → wait for approval → ship.
- Could slow release cadence. Plan for prospectus lead time in release planning.
- v1.7.0-alpha features (immersive mode, footnotes, etc.) are incremental — likely not "significantly different."
- Bigger features (new platforms, user notes, AI features) would definitely trigger this.

**Workflow proposal:**
- Create a prospectus template document
- Add checklist item to release process: "Draft and submit Biblica prospectus"
- Submit at least 2-4 weeks before target release date

---

## 9. Attribution & Branding (§7–8)

### Biblica Link (§7.C.1)
> "ORGANIZATION shall display at all times, a direct one-step link to LICENSOR'S website (currently www.Biblica.com) and which shall include the word 'Biblica' somewhere in the text of the link, every time the TRANSLATIONS are displayed."

**Impact:**
- ✅ Could integrate into the About screen, a footer in Settings, or a persistent overlay.
- The link must be visible **whenever NVI content is shown**, not just in a menu.
- Should be part of the copyright notice display.

**Implementation idea:**
- In ReaderScreen or the UI, show a footer bar with copyright + "Read the Bible at Biblica.com" link.
- Only shown when using the online NVI source.

### Usage Statistics (§7.C.2)
> "ORGANIZATION permits LICENSOR and its affiliates and subcontractors to obtain accurate and complete user statistics reports from the API for the CONTENT in the WORK."

**Impact:**
- ✅ This is handled automatically by the YouVersion API — they track API calls.
- We don't need to implement anything; the API already provides this data to Biblica.

### Copyright & Trademark Notice (§8.B)
Must display:
1. Sound recording copyright notice (3 lines)
2. Text copyright notice (3 lines)
3. Trademark notice

Per the most recent API metatext, following this template:

```
[Insert three-line sound recording copyright notice]
[Insert three-line text copyright notice]
[Insert trademark notice]
```

**Impact:**
- Must fetch the notice text from API metatext (already available in the API response)
- Display whenever NVI content is visible
- The Biblica link can be part of this notice

---

## 10. Term & Renewal (§4)

- **Initial term**: 2 years from effective date
- **Auto-renews**: successive 2-year terms
- Either party can terminate

**Impact:**
- If we sign now, it's valid through ~2028.
- Auto-renewal is convenient but set a calendar reminder to review terms before each renewal.

---

## 11. Termination Triggers (§13)

| Clause | Trigger | Cure Period | Outcome |
|--------|---------|-------------|---------|
| **13.A — Mission clause** | App is "seriously and demonstrably anti-Christian" or doesn't "glorify Jesus Christ and promote biblical principles" (Biblica's sole discretion) | None (immediate) | License terminated |
| **13.B — Change of control** | Organization is sold/merged/reorganized | 30d notice | Biblica may terminate |
| **13.C — Default** | Any breach of agreement, including failure to provide usage reports or promotional opportunities | **7 days** to cure | License terminated; Biblica may ban future applications |
| **13.D — SDK termination** | YouVersion SDK License terminates | Immediate | Auto-termination |
| **13.E — Development deadline** | **Failure to complete development and deploy within 12 months** | None | Immediate termination |
| **13.G — Bankruptcy** | Either party files for bankruptcy | Immediate | Termination |

### ⚠️ KEY CONCERN: 12-Month Deployment Clause (§13.E)

> "LICENSOR may terminate this Agreement immediately if ORGANIZATION fails to complete the development of and commence deployment of the CONTENT within the WORK within twelve (12) months of this Agreement."

**This is a major issue for a solo developer.** TheWord is still in early alpha (v1.7.0-alpha). We will likely not ship a production release with NVI within 12 months.

**Possible approaches:**
1. **Defer signing until closer to production** — only sign the NVI license when we're ready to ship it (e.g., v1.0.0 or a stable release).
2. **Ship NVI in a limited/incomplete form** within 12 months to satisfy the clause, then iterate. Risk: clause 5.K (prospectus) could complicate this.
3. **Contact YouVersion/Biblica** and explain the solo dev timeline — they may grant an extension or clarify that alpha/beta deployment counts.
4. **Use BSB (English, ID 3034) for now** — it's already working and doesn't require a Biblica license. Only sign NVI license when ready.

**Recommended path:**
- Stick with **BSB + USFM** for current development.
- Defer NVI license application until we're within ~6 months of a production release.
- Document the prospectus workflow now so it's ready when needed.

---

## 12. Post-Termination Obligations (§13.H)

If the agreement ends for any reason:
- **72 hours** to destroy all production materials and database files containing CONTENT
- **72 hours** to remove all non-physical editions from public access (app stores, etc.)
- Written notification signed by officer confirming both actions

**Impact:**
- If the license is terminated (for any reason), we have 3 days to:
  - Delete any cached/stored NVI content
  - Pull the app from stores
  - Send a written confirmation
- This is feasible but requires a clear plan.

---

## 13. Legal Provisions

| Clause | Summary |
|--------|---------|
| **Indemnification (§10)** | Mutual indemnification for breaches. Organization indemnifies Licensor for distribution claims. |
| **Dispute Resolution (§11)** | Meeting → Mediation → Binding Arbitration, all in Colorado Springs, CO, USA. |
| **Governing Law (§12)** | Colorado, USA. |
| **Audit Rights (§14)** | Biblica can inspect books/records once/12 months on 30 days notice. |
| **Confidentiality (§19)** | Agreement terms are strictly confidential — cannot share publicly. |
| **No Partnership (§15)** | No employer/employee, joint venture, or partnership created. |

---

## 14. Biblica's Mission Statement (§13.A)

> "The purpose and passion of Biblica, Inc., is to provide the Bible in accurate, contemporary translations and formats so that more people around the world will have the opportunity to be transformed by Jesus Christ."

This mission clause is used as the standard for termination decisions. Worth keeping in mind as the project evolves.

---

## Summary: Changes Needed for Compliance

| # | Change | Priority |
|---|--------|----------|
| 1 | Remove `StripFootnotes()` in `BibleClient::ParseHtmlChapter()` | 🔴 Required before NVI use |
| 2 | Implement footnote rendering in the UI | 🔴 Required (already planned) |
| 3 | Add copyright/trademark notice overlay when NVI content is displayed | 🔴 Required |
| 4 | Add "Biblica" direct one-step link visible with NVI content | 🔴 Required |
| 5 | Ensure no raw API content is cached persistently to disk | 🔴 Required |
| 6 | Audit encryption of any stored content | 🟡 Needed |
| 7 | Implement 2-chapter display limit in continuous scrolling | 🟡 Needed if we add that feature |
| 8 | Create prospectus template and release workflow | 🟡 Before first NVI release |
| 9 | Sign Biblica agreement | 🟡 Only when ready to ship (~6 months before target release) |

---

## Decision

**For now**: Stick with **BSB (3034)** via API and **Bíblia Livre** via USFM. Both are free/public domain and require no special license. The NVI license is a future concern — defer signing until we're within 6 months of a production release that would include NVI as a selectable translation.

**12-month clause** is the main blocker for early adoption. As a solo developer, TheWord will not ship a production release with NVI within one year of signing. Best to wait.
