# moekuri-trans

MoeKuri 2 日文 1.10 的 UTF-8 翻译编辑、提取和单 EXE 打包工具。C++17 · Win32 x86 · Windows 8+

UTF-8 translation editor, extractor and single-EXE packager for the Japanese 1.10 release of MoeKuri 2. Dynamic text uses bounded template matching, complete string-argument translation, kana-width normalization and preserved line boundaries, based on the local MOD implementation.

`catalog-overrides.json` records four title/numeral corrections and preserves 17 script identifiers. Standalone `@` identifiers such as `@ブラリボン` remain Japanese and are excluded from the runtime lookup table; visible names such as `ブラリボン` can still be translated. Story commands are preserved; encyclopedia descriptions and dialogue after `#txt@` are translated before rendering, so typewriter text starts in the target language.

`@ブラリボン` 等独立 `@` 标识符保留日文，并排除出运行时替换表；显示名称 `ブラリボン` 仍可译为“布拉玛（缎带）”。剧情命令不翻译，图鉴介绍和 `#txt@` 后的对白在读取时翻译，剧情从第一个字起直接显示译文。

[中文](#zh-简体中文) · [English](#en-english) · [한국어](#ko-한국어) · [Français](#fr-français) · [日本語](#ja-日本語) · [Italiano](#it-italiano) · [Deutsch](#de-deutsch) · [Русский](#ru-русский) · [العربية](#ar-العربية) · [Español](#es-español)

```text
moekuri-trans/
├── src/                    C++ GUI, archive readers, PE packager and runtime
├── tests/                  Python regression and font coverage checks
├── .github/workflows/      Windows CI and tag-based Releases
├── build.bat / build.ps1    One-click compile; -Package builds a minimal release
├── package-release.ps1     Strict release file allowlist
├── build.cmd               MSVC x86 build
├── install-build.ps1       Install the built tool and keep previous EXEs
├── 1.10trans.txt           JP 1.10 Simplified Chinese import example
├── 1.10trans.csv           Equivalent UTF-8 BOM CSV example (not duplicated in Release)
├── 1.10trans-report.json   Reference coverage and provenance
├── translation.txt         Local working catalog
├── catalog-overrides.json   Reviewed titles and protected script identifiers
├── font.ttf                 Default Xiaolai font (tracked and distributed)
├── FONT-LICENSE-OFL.txt     Font license
├── FONT-NOTICES.txt         Font attribution
├── .gitignore
└── README.md
```

Local/generated files: `MoeKuriTools.exe`, `build/`, `tools-settings.json`, dist/, test reports and exported catalogs. The current workspace keeps this project in `moekuri求助/1.10/tools/`.

## zh-简体中文

### 1.10 导入样例与 EXE 标记

在工具中点击“导入词库”，选择 `1.10trans.txt` 或 `1.10trans.csv`。两份样例都是简体中文词库，包含 **16,808 条**，覆盖参考汉化目录中的全部 **13,084 条**，并补入地形附加效果、显示标签、剧情拆行及半角/换行变体；署名和同形术语保留原文。TXT 为 UTF-8，CSV 为带 BOM 的 UTF-8。默认先读取本地 `translation.txt`，没有时加载 `1.10trans.txt`；精简 Release 只附 TXT 样例，CSV 可通过“导出词库”生成。

生成的游戏 EXE 包含独立只读 PE 区段 **`.mktrans`**。其开头是明文 ASCII `moekuri_trans` 加 NUL，随后为 UTF-8 JSON，记录标记版本、原版 EXE SHA256、词库 SHA256、条目数及字体模式。可在 PE 查看器中找到该区段，或运行 `MoeKuriTools.exe --inspect 翻译版.exe` 检查。

### 字体来源与致谢

项目默认字体是 **小赖字体（Xiaolai Font）**。感谢 LXGW（落霞孤鹜）维护小赖字体，感谢 Nozomi Seto 创作上游 SetoFont（濑户字体）。

[项目地址: lxgw/kose-font](https://github.com/lxgw/kose-font) · [字体许可证: SIL Open Font License 1.1](https://github.com/lxgw/kose-font/blob/master/OFL.txt) · [FONT-LICENSE-OFL.txt](FONT-LICENSE-OFL.txt) · [FONT-NOTICES.txt](FONT-NOTICES.txt)

### 外置字体与发布

勾选 **[外置字体]** 后，运行时从游戏 EXE 所在目录依次尝试 `font.otf`、`font.ttf`，读取文件内部的实际字体名称；文件缺失或无效时使用系统字体。此模式不内嵌字体。外置选项默认关闭，字体优化默认开启。CLI 使用 `--external-font 1`。图鉴完整介绍在原生文本读取阶段翻译，再由游戏拆行显示。

一键构建：`build.bat` 或 `powershell -NoProfile -ExecutionPolicy Bypass -File build.ps1`；加 `-Package` 会编译、检查依赖并生成发布包。上传本项目目录的内容作为 GitHub 仓库根目录，推送 `v*` 标签后自动编译并发布 Release；普通提交和 PR 只构建，Actions 手动运行可指定已有标签。发布附件包括 ZIP、独立的 `font.ttf` 和 SHA256SUMS.txt；ZIP 收录工具 EXE、README、1.10trans.txt、默认小赖字体 `font.ttf` 和两份字体许可/通知。默认字体随仓库保存，打包时也复制到 `dist/font.ttf`，校验文件覆盖 ZIP 和字体。原游戏、其他本机字体、构建缓存、测试数据、设置和备份仍不打包。Windows 系统 DLL 由系统提供；翻译运行时 DLL 在构建时内嵌到工具，随后打入游戏 EXE。

[build.ps1](build.ps1) · [package-release.ps1](package-release.ps1) · [Windows workflow](.github/workflows/windows-release.yml)

启动本目录的 `MoeKuriTools.exe`。更新后请关闭旧窗口再重新启动；打开着的旧程序不会自动变成新版。工具自身是独立 C++ Win32 EXE，生成的游戏也是单 EXE，字体和词库直接嵌入其中。压缩功能使用 Windows 自带的 Compression API，需要 Windows 8 或更新版本。

### 性能与浏览

卡住时的线程栈显示，Windows 辅助功能查询正在 UI 线程遍历全部 13,076 个列表项。现改为每页最多 80 条，使用“上一页 / 下一页”浏览，或者输入页码并点击“跳转”/按 Enter；搜索仍覆盖全部词库。编辑单条文本只验证当前条目，避免每次选择都复制、验证整份词库。启动加载、提取、导入、保存、打包和字体读取由后台线程处理，搜索延迟 180 毫秒合并输入，系统字体列表在首次展开时后台读取。

### 使用步骤

1. 默认读取 `translation.txt`，也可以点“导入词库”选择 TXT、TSV、CSV 或 JSON。
2. 选中一条，在右侧编辑原文、译文及本条宽度。点击“应用修改”，或切换条目时自动提交。原文必须准确匹配，空译文保留原文；“只看未翻译”也包含译文与原文相同的条目。
3. “导出词库”支持四种格式，在保存对话框的文件类型栏选择。
4. 选择未修改的日文 1.10 游戏 EXE。默认会自动识别版本。选择 TTF/OTF 后自动读取它的字体家族名；清除字体文件后，可以从字体名称栏选择系统字体。
5. 点击“生成翻译 EXE”。默认输出在所选游戏 EXE 所在目录，文件名为 `moekuri_translate.exe`；原版 EXE 不会被覆盖。
6. “从 EXE 解包”恢复词库、布局参数和内嵌字体；字体保存在内存，可以重新打包或点“导出字体”保存。

### 体积优化

“字体优化（仅嵌入用到的字形）”是默认勾选的复选框选项。生成 EXE 时根据当前词库的原文/译文、可提取的原版 EXE 和邻近 DXA 文本，以及基本数字、拉丁字符、日文假名和标点精简字体；保留所需字形及复合字形依赖。原始 TTF/OTF 文件不变。对不支持精简的字体格式，以及词库译文包含复杂连写文字的情况，保留完整字体。关闭该选项可强制嵌入完整字体。

字体、查找数据及原始 UTF-8 词库一起进行 XPRESS Huffman 无损压缩，运行时在内存解压并设为只读，不生成临时文件，也不需要额外压缩 DLL 文件。新工具同时可解包旧的未压缩 EXE。

当前 16,808 条词库 + Xiaolai 字体，测试版由 29,322,752 字节（27.96 MiB）降至 9,145,856 字节（8.72 MiB），约减少 69%；内嵌字体由 22,220,806 字节精简到 4,399,724 字节。已对照完整字体检查词库及可提取文本所需字符的字形覆盖，并通过加载和字体注册探针。

精简版解包得到的是精简后的字体。如果新增了原先未保留的字符，请重新选择完整字体文件后生成；对于无法提取的动态内容，也可关闭字体优化保留完整覆盖。

语言栏提供中文、英语、韩语、法语、日语、意大利语、德语、俄语、阿拉伯语和西班牙语。首次启动根据 Windows 用户界面语言选择；其他系统语言回退英语。手动选择保存到 `tools-settings.json`，以后优先使用保存的选择。工具语言切换不会自动翻译游戏词库。

### UTF-8 文件格式

TXT / TSV 每个物理行是一条记录，列之间使用真正的 TAB：

```text
原文<TAB>译文<TAB>可选宽度像素
```

上面的 `<TAB>` 只是说明，实际文件用制表符。文本内部的换行、回车、TAB 和反斜线分别写成 `\n`、`\r`、`\t`、`\\`。`#` 开头的行是注释。推荐从工具导出样本，再进行编辑。未经编辑的 TXT 直接打包后，解包可以逐字节恢复，包括 BOM、换行和注释。

CSV 使用 `Original,Translation,WidthPixels` 表头；前两列必需，宽度可选。也接受 `Source/Target` 和 `原文/译文` 表头。CSV 单元格中直接存真实换行，双引号按照 CSV 规则写成两个双引号；反斜线是普通字符，不做 TXT 转义。导出为带 BOM 的 UTF-8，便于 Excel 识别。

JSON 兼容之前的扁平字典 `{"原文":"译文"}`。这种格式不保存每条的宽度预算；需要保留宽度请用 TXT、TSV 或 CSV。输入统一要求 UTF-8，UTF-16/GBK 文件应先转换。所有格式都会检查重复原文、嵌入 NUL、超长文本和常见 printf 占位符类型/顺序。

### 字体及宽度

“高度偏移”和“宽度偏移”对应旧 `脚本/更改字体/SRL.ini` 的 `Height` / `Width`，是加减偏移值，默认均为 0。文字缩放默认 100%。默认宽度 620 px；设为 0 关闭自动宽度适配。本条宽度 0 使用默认值，最小横向缩放默认 70%。横向适配也用于文字宽度计算，避免显示和测量使用不同译文。

提供的 `font.ttf` 是 Xiaolai，相关授权和通知见 `FONT-LICENSE-OFL.txt`、`FONT-NOTICES.txt`。生成 EXE 后运行时不再读取本目录的字体或词库。游戏仍需要原版的 DXA、音频等资源文件，生成 EXE 应放在原版游戏目录。

### 单 EXE 的实现与测试范围

仅接受 SHA256 为 `1c79a2d328d8ccd69765ac624b48f2317d80688c3dc7439ee3c23fc1fc05f847` 的原版 1.10 EXE。压缩后的译文、原始 UTF-8 词库和字体放到新的 `.cnro` 只读区段，解压内容和运行时转换出的查找表位于独立内存并改成只读；游戏原有文字数据及其 RVA 保留。通过已确认的 1.10 文字绘制/测量入口重定向读取，不在原有字符串槽内按长度替换。存储和编辑用 UTF-8，原引擎调用接口仍是 UTF-16。

自动回归见 `tests/regression-report.json`，运行 `python tests/regression.py` 可重新检查。已覆盖完整词库格式转换、100 次选择编辑、100 次滚动、所有分页及页码跳转、十种界面语言、Windows 辅助功能批量查询、TXT 逐字节解包、精简字体字形覆盖、关闭字体优化时字体逐字节解包、压缩校验和边界检查，以及实际 PE 加载和运行时探针。回归先写入 `build/optimized.exe`，便于测试时主目录游戏仍然运行。探针使用替代绘制后端核对翻译及宽度调用；尚未完成真实游戏场景、战斗与完整剧情的人工验收。提取结果包含候选字符串，需要人工筛选，现有译文也需要按 1.10 实际画面校对。

### 命令行与编译

```powershell
.\MoeKuriTools.exe --help
.\MoeKuriTools.exe --validate translation.txt
.\MoeKuriTools.exe --export-csv translation.txt translation.csv
.\MoeKuriTools.exe --import-csv translation.csv translation.txt
.\MoeKuriTools.exe --convert translation.csv translation.tsv
.\MoeKuriTools.exe --extract "原版.exe" originals.txt
.\MoeKuriTools.exe --pack "原版.exe" translation.txt "翻译版.exe" --font font.ttf --subset-font 1 --height 0 --width 0 --scale 100 --max-width 620 --min-scale 70
.\MoeKuriTools.exe --inspect "翻译版.exe"
.\MoeKuriTools.exe --unpack "翻译版.exe" unpacked.txt --font-out unpacked.ttf --settings-out unpacked-settings.json
```

源代码在 `src`。使用 VS 2022 的 C++ x86 工具链与 Windows SDK 运行 `build.cmd`。运行时模块在编译时嵌入工具，生成的游戏不依赖这个模块文件。构建会保留旧 EXE 备份，不强行结束已打开的编辑窗口。

```powershell
cmd /c build.cmd
python tests/regression.py
```

编译需要安装 VS 2022 的“使用 C++ 的桌面开发”组件及 Windows SDK。测试脚本只依赖 Python 3 标准库。完整回归需要把支持的原版游戏 EXE 和资源放在项目目录的上一级，并在项目目录准备 `translation.txt`、`font.ttf`。默认 `font.ttf` 随仓库保存，其他本地字体不会提交到 Git；可在构建后选择自己的 TTF/OTF。Xiaolai 的来源和许可见 [字体通知](FONT-NOTICES.txt) 与 [OFL 许可](FONT-LICENSE-OFL.txt)。

`.gitignore` 保留源码、测试脚本、文档、字体许可和 `translation.txt`，排除编译产物、字体文件、设置、导出的 CSV、提取结果及测试输出。原游戏由使用者自行准备。本项目尚未单独指定源码许可证；字体许可只适用于相应字体。

## en-English

### 1.10 import example and EXE marker

Use Import catalog to open `1.10trans.txt` or `1.10trans.csv`. Both contain **16,808 Simplified Chinese entries**, covering all **13,084 reference entries**, plus terrain effects, display labels and exact line/Unicode-width variants. Credits and identical terms retain their source spelling. TXT is UTF-8; CSV is UTF-8 with BOM. Startup loads local `translation.txt`, falling back to `1.10trans.txt`. The minimal release ships only the TXT example; export CSV with the tool.

Generated game EXEs have a read-only **`.mktrans`** PE section containing ASCII `moekuri_trans`, NUL, then UTF-8 JSON: marker version, original EXE and catalog SHA256, row count and font mode. Inspect with a PE viewer or `MoeKuriTools.exe --inspect translated.exe`.

### Font source and acknowledgements

The project’s default font is **小赖字体 / Xiaolai Font**. Thanks to LXGW for Xiaolai and Nozomi Seto for its upstream SetoFont.

[Project: lxgw/kose-font](https://github.com/lxgw/kose-font) · [Font license: SIL Open Font License 1.1](https://github.com/lxgw/kose-font/blob/master/OFL.txt) · [FONT-LICENSE-OFL.txt](FONT-LICENSE-OFL.txt) · [FONT-NOTICES.txt](FONT-NOTICES.txt)

### External font and releases

Enable **[External font]** to try `font.otf`, then `font.ttf`, beside the game EXE, falling back to the system font. The file’s actual family name is detected; no font is embedded. External mode defaults off; font optimization defaults on. CLI: `--external-font 1`. Complete encyclopedia descriptions are translated before the game wraps lines.

Build with `build.bat` or `build.ps1`; `-Package` runs dependency checks and creates the release ZIP. Use this project directory as the GitHub repository root. Pushing `v*` tags publishes releases; ordinary commits and PRs only build. ZIP contents: tool EXE, README, 1.10trans.txt, the default Xiaolai `font.ttf` and both font notices/license files. The default font is tracked in Git, copied to `dist/font.ttf` and published as a separate Release asset alongside the ZIP and SHA256SUMS.txt. Checksums cover both assets. Game assets, other local fonts, caches, settings and backups are excluded. The runtime DLL is embedded; remaining DLL dependencies are Windows components.

[build.ps1](build.ps1) · [package-release.ps1](package-release.ps1) · [Windows workflow](.github/workflows/windows-release.yml)

**moekuri-trans** is a C++17/Win32 tool for editing, extracting, packing and unpacking translations for the unmodified Japanese 1.10 game. It creates a single translated EXE containing the catalog and optional font; the original game data and audio are still required. Windows 8 or later is required.

1. Launch `MoeKuriTools.exe` and choose **Import catalog** to open UTF-8 TXT, TSV, CSV or flat JSON.
2. Select a row, edit its source, translation and optional width, then apply the change. An empty translation keeps the original text. **Export catalog** saves the selected file format.
3. Browse 80 rows per page or enter a page number and click **Go** / press Enter. Search covers the entire catalog. Loading and packaging run in the background.
4. Select the original game EXE and optionally a TTF/OTF font. **Optimize font** is checked by default: only required glyphs are embedded. Uncheck it to retain the full font. The input font file remains unchanged.
5. Choose **Build translated EXE** and place the result beside the original game assets. **Unpack EXE** restores its catalog, layout settings and embedded font; **Export font** saves that font.

The first launch follows the Windows UI language, with English as fallback. Manual language selection is saved. Changing the interface language does not translate the catalog. Restart after updating the tool.

TXT/TSV use source, translation and optional pixel width separated by actual TAB characters; escape internal newlines, tabs and backslashes as `\n`, `\t` and `\\`. CSV uses `Original,Translation,WidthPixels`, standard CSV quoting and real multiline cells; export includes a UTF-8 BOM. Flat JSON stores text pairs and loses per-row widths. Height/width offsets match SRL.ini, default text scale is 100%, default width is 620 px, and minimum horizontal scale is 70%; width 0 disables automatic fitting.

Fonts and text are compressed losslessly and decoded into read-only memory. The current fixture shrank from 27.96 MiB to 8.72 MiB. If you add characters after unpacking a subset font, select a complete font again. Unsupported font formats and detected complex shaping in translations retain the full font.

Build with **Visual Studio 2022**, the **Desktop development with C++** workload and a **Windows SDK**, using `cmd /c build.cmd`. Run `python tests/regression.py` with Python 3; full regression needs the original EXE/assets in the parent directory and local `translation.txt`/`font.ttf`. Reports are generated under `tests/`. Other local fonts, binaries, settings and test output are ignored by Git; source, test scripts, licenses and `translation.txt` are retained. See [font notices](FONT-NOTICES.txt) and [OFL](FONT-LICENSE-OFL.txt). No separate source-code license has been assigned.

Only the original EXE with SHA256 `1c79a2d328d8ccd69765ac624b48f2317d80688c3dc7439ee3c23fc1fc05f847` is supported. Stored text is UTF-8; the engine interface remains UTF-16. Regression covers formats, pagination, editing, accessibility, glyph coverage, decompression and loader probes. Real battles and the complete story still need manual acceptance; extracted strings are candidates requiring review.

## ko-한국어

### 1.10 가져오기 예제 및 EXE 표시

번역 목록 가져오기에서 `1.10trans.txt` 또는 `1.10trans.csv`를 선택하세요. 중국어 간체 **16,808개 항목**이며 참고 번역 **13,084개**와 지형 효과·표시 문자열·줄바꿈·문자 폭 변형을 포함합니다. 이름과 같은 표기 용어는 유지됩니다. TXT는 UTF-8, CSV는 BOM 포함 UTF-8입니다. 시작 시 `translation.txt`, 없으면 `1.10trans.txt`를 읽습니다. 최소 배포판에는 TXT만 포함되고 CSV는 도구로 내보낼 수 있습니다.

생성된 게임 EXE의 읽기 전용 **`.mktrans`** PE 섹션에는 ASCII `moekuri_trans`, NUL, UTF-8 JSON이 들어 있습니다. 버전, 원본 EXE/번역 목록 SHA256, 항목 수, 글꼴 모드를 기록합니다. PE 뷰어 또는 `MoeKuriTools.exe --inspect translated.exe`로 확인하세요.

### 글꼴 출처 및 감사

프로젝트 기본 글꼴은 **小赖字体 / Xiaolai Font**입니다. Xiaolai의 LXGW와 원본 SetoFont의 Nozomi Seto에게 감사드립니다.

[프로젝트: lxgw/kose-font](https://github.com/lxgw/kose-font) · [글꼴 라이선스: SIL Open Font License 1.1](https://github.com/lxgw/kose-font/blob/master/OFL.txt) · [FONT-LICENSE-OFL.txt](FONT-LICENSE-OFL.txt) · [FONT-NOTICES.txt](FONT-NOTICES.txt)

### 외부 글꼴 및 배포

**[외부 글꼴]**은 게임 EXE 폴더의 `font.otf`, `font.ttf` 순서로 시도하고, 없거나 잘못된 파일이면 시스템 글꼴을 사용합니다. 실제 글꼴 이름을 읽으며 포함하지 않습니다. 외부 모드는 기본 해제, 글꼴 최적화는 기본 선택입니다. CLI: `--external-font 1`. 도감 설명은 줄바꿈 전에 번역됩니다.

`build.bat` 또는 `build.ps1`로 빌드하고 `-Package`로 검사·패키징합니다. 이 프로젝트 폴더를 GitHub 저장소 루트로 사용하세요. `v*` 태그를 푸시하면 Release가 생성됩니다. ZIP에는 도구 EXE, README, 1.10trans.txt, 기본 Xiaolai `font.ttf`, 라이선스와 안내가 포함됩니다. 기본 글꼴은 Git에 보존하고 `dist/font.ttf`에도 복사합니다. Release는 ZIP, 별도 `font.ttf`, 두 파일의 SHA256SUMS.txt를 제공합니다. 게임 자료·다른 로컬 글꼴·캐시·설정·백업은 제외됩니다. 런타임 DLL은 포함되며 나머지는 Windows 구성 요소입니다.

[build.ps1](build.ps1) · [package-release.ps1](package-release.ps1) · [Windows workflow](.github/workflows/windows-release.yml)

**moekuri-trans**는 수정되지 않은 일본어 1.10 게임용 C++17/Win32 번역 편집·추출·패키징 도구입니다. Windows 8 이상이 필요합니다. 번역과 글꼴을 하나의 EXE에 포함하지만 원래 게임 데이터와 오디오 파일은 필요합니다.

1. `MoeKuriTools.exe`에서 번역 목록을 가져오세요. UTF-8 TXT·TSV·CSV·JSON을 지원합니다.
2. 원문·번역·항목별 너비를 편집하고 적용한 뒤 원하는 형식으로 내보내세요. 빈 번역은 원문을 유지합니다.
3. 페이지당 80개 항목을 표시합니다. 페이지 번호를 입력하고 이동 버튼 또는 Enter를 누르세요. 검색은 전체 목록을 대상으로 합니다.
4. 원본 EXE와 선택적인 TTF/OTF를 지정하세요. **글꼴 최적화는 기본적으로 켜져 있으며**, 필요한 글리프만 포함합니다. 끄면 전체 글꼴을 포함합니다. 원본 글꼴 파일은 변경되지 않습니다.
5. 번역 EXE를 생성하여 원래 게임 리소스 옆에 두세요. EXE 추출은 목록·설정·내장 글꼴을 복원하며, 글꼴도 별도로 내보낼 수 있습니다.

처음에는 Windows 표시 언어를 사용하고 지원하지 않는 언어는 영어로 표시합니다. 수동 선택은 저장되며 UI 언어 변경이 번역 목록을 자동 번역하지는 않습니다. 업데이트 후 다시 실행하세요.

TXT/TSV는 실제 TAB으로 열을 나누고 내부 줄바꿈·탭·역슬래시는 `\n`, `\t`, `\\`로 씁니다. CSV 열은 `Original,Translation,WidthPixels`이며 실제 여러 줄 셀과 표준 따옴표 규칙을 사용합니다. JSON은 항목별 너비를 보존하지 않습니다. 기본 배율 100%, 너비 620 px, 최소 가로 배율 70%이며 너비 0은 자동 맞춤을 끕니다. 정밀화된 글꼴을 추출한 뒤 새 문자를 추가하려면 전체 글꼴을 다시 선택하세요.

VS 2022의 C++ 데스크톱 개발 구성 요소와 Windows SDK를 설치하고 `cmd /c build.cmd`로 빌드하세요. Python 3에서 `python tests/regression.py`를 실행할 수 있으며 전체 검증에는 상위 폴더의 원본 게임과 로컬 `translation.txt`·`font.ttf`가 필요합니다. 소스·테스트 스크립트·번역 원본은 Git에 보존하고 다른 로컬 글꼴·바이너리·설정·테스트 출력은 제외합니다. [글꼴 안내](FONT-NOTICES.txt)와 [OFL](FONT-LICENSE-OFL.txt)을 확인하세요. 별도의 소스 코드 라이선스는 아직 지정되지 않았습니다.

현재 테스트 파일은 무손실 압축과 글꼴 최적화로 27.96 MiB에서 8.72 MiB로 줄었습니다. 형식·페이지 이동·글리프·로딩 검사는 통과했지만 실제 전투와 전체 스토리는 수동 검증이 필요합니다. 추출된 문자열도 검토해야 합니다.

## fr-Français

### Exemple 1.10 et marqueur EXE

Importer catalogue ouvre `1.10trans.txt` ou `1.10trans.csv` : **16 808 entrées en chinois simplifié**, dont les **13 084 entrées de référence**, effets de terrain et variantes d’affichage, de lignes et de largeur Unicode. Les noms crédités et termes identiques restent inchangés. TXT : UTF-8 ; CSV : UTF-8 avec BOM. Au démarrage, `translation.txt` est prioritaire, sinon `1.10trans.txt`. La Release minimale contient le TXT ; exporter le CSV depuis l’outil.

Les EXE créés contiennent une section PE en lecture seule **`.mktrans`** : ASCII `moekuri_trans`, NUL puis JSON UTF-8 avec version, SHA256 du jeu original et du catalogue, nombre d’entrées et mode de police. Utiliser un lecteur PE ou `MoeKuriTools.exe --inspect translated.exe`.

### Police et remerciements

La police par défaut est **小赖字体 / Xiaolai Font**. Merci à LXGW pour Xiaolai et à Nozomi Seto pour SetoFont.

[Projet: lxgw/kose-font](https://github.com/lxgw/kose-font) · [Licence de la police: SIL Open Font License 1.1](https://github.com/lxgw/kose-font/blob/master/OFL.txt) · [FONT-LICENSE-OFL.txt](FONT-LICENSE-OFL.txt) · [FONT-NOTICES.txt](FONT-NOTICES.txt)

### Police externe et publication

**[Police externe]** essaie `font.otf`, puis `font.ttf`, dans le dossier EXE du jeu, puis la police système. Le nom réel est détecté ; aucune police intégrée. Mode externe désactivé par défaut, optimisation activée. CLI : `--external-font 1`. Les descriptions du bestiaire sont traduites avant le découpage des lignes.

Compiler avec `build.bat` ou `build.ps1` ; `-Package` vérifie les dépendances et crée le ZIP. Ce dossier constitue la racine du dépôt GitHub. Les tags `v*` déclenchent une Release. Le ZIP contient EXE, README, 1.10trans.txt, la police Xiaolai `font.ttf` et les deux fichiers de licence/mentions. La police par défaut est conservée dans Git, copiée vers `dist/font.ttf` et publiée séparément avec le ZIP et SHA256SUMS.txt. Les deux fichiers sont vérifiés. Ressources du jeu, autres polices locales, caches, réglages et sauvegardes sont exclus. La DLL interne est intégrée ; les autres DLL sont des composants Windows.

[build.ps1](build.ps1) · [package-release.ps1](package-release.ps1) · [Windows workflow](.github/workflows/windows-release.yml)

**moekuri-trans** est un outil C++17/Win32 d’édition, d’extraction et de création d’un EXE traduit pour la version japonaise 1.10 non modifiée. Windows 8 ou ultérieur est requis. L’EXE contient les textes et la police choisie ; les données et l’audio du jeu restent nécessaires.

1. Lancez `MoeKuriTools.exe` et importez un catalogue UTF-8 TXT, TSV, CSV ou JSON.
2. Modifiez le texte source, la traduction et la largeur éventuelle, puis appliquez et exportez. Une traduction vide conserve l’original.
3. Parcourez 80 entrées par page ou saisissez un numéro et cliquez sur Aller / appuyez sur Entrée. La recherche couvre tout le catalogue.
4. Sélectionnez l’EXE original et éventuellement une police TTF/OTF. **L’optimisation de la police est cochée par défaut** : seuls les glyphes nécessaires sont inclus. Décochez-la pour conserver toute la police. Le fichier source reste intact.
5. Créez l’EXE traduit et placez-le près des ressources originales. L’extraction d’un EXE restaure le catalogue, les paramètres et la police intégrée, qui peut être exportée.

La langue initiale suit Windows, sinon l’anglais ; le choix manuel est mémorisé. La langue de l’interface ne traduit pas le catalogue. Relancez l’outil après une mise à jour.

TXT/TSV séparent les colonnes par une vraie tabulation ; les caractères internes utilisent `\n`, `\t`, `\\`. CSV utilise `Original,Translation,WidthPixels`, des cellules multilignes réelles et les guillemets CSV standards. Le JSON plat perd les largeurs individuelles. Valeurs par défaut : échelle 100 %, largeur 620 px, minimum horizontal 70 % ; largeur 0 désactive l’ajustement. Après extraction d’une police réduite, choisissez une police complète si vous ajoutez des caractères.

Installez VS 2022 avec le développement desktop C++ et un Windows SDK, puis exécutez `cmd /c build.cmd`. Les tests utilisent Python 3 : `python tests/regression.py`. La régression complète nécessite le jeu original dans le dossier parent et `translation.txt`/`font.ttf` localement. Git conserve sources, scripts, licences et catalogue TXT ; les autres polices locales, binaires, paramètres et sorties de test sont ignorés. Consultez [les notices](FONT-NOTICES.txt) et [l’OFL](FONT-LICENSE-OFL.txt). Aucune licence distincte du code source n’a été attribuée.

Le cas testé passe de 27,96 à 8,72 MiB avec réduction de police et compression sans perte. Les formats, pages, glyphes et sondes de chargement sont vérifiés ; les combats et l’histoire complète attendent une validation manuelle. Les chaînes extraites sont des candidates à examiner.

## ja-日本語

### 1.10 辞書サンプルと EXE マーカー

「辞書を読み込み」で `1.10trans.txt` または `1.10trans.csv` を選びます。簡体字中国語 **16,808 項目**で、参照辞書の **13,084 項目**と、地形の追加効果、表示ラベル、改行・文字幅の差を含みます。クレジット名と同形語は保持します。TXT は UTF-8、CSV は BOM 付き UTF-8。起動時は `translation.txt`、なければ `1.10trans.txt` を読みます。最小リリースには TXT のみを収録し、CSV はツールから出力できます。

生成したゲーム EXE の読み取り専用 PE セクション **`.mktrans`** には ASCII `moekuri_trans`、NUL、UTF-8 JSON を格納します。バージョン、原版 EXE と辞書の SHA256、項目数、フォントモードを記録します。PE ビューアーまたは `MoeKuriTools.exe --inspect translated.exe` で確認できます。

### フォントと謝辞

既定のフォントは **小赖字体 / Xiaolai Font** です。Xiaolai の LXGW 氏と、元の SetoFont を制作した Nozomi Seto 氏に感謝します。

[プロジェクト: lxgw/kose-font](https://github.com/lxgw/kose-font) · [フォントのライセンス: SIL Open Font License 1.1](https://github.com/lxgw/kose-font/blob/master/OFL.txt) · [FONT-LICENSE-OFL.txt](FONT-LICENSE-OFL.txt) · [FONT-NOTICES.txt](FONT-NOTICES.txt)

### 外部フォントとリリース

**[外部フォント]** はゲーム EXE の場所で `font.otf`、`font.ttf` の順に試し、無効または見つからない場合はシステムフォントを使います。実際のフォント名を検出し、埋め込みません。外部モードは既定で無効、最適化は有効です。CLI: `--external-font 1`。図鑑説明は改行処理より前に翻訳します。

`build.bat` または `build.ps1` でビルドし、`-Package` で依存関係を検査して ZIP を作成します。このディレクトリを GitHub リポジトリのルートにします。`v*` タグの push で Release が作成されます。ZIP にはツール EXE、README、1.10trans.txt、標準の Xiaolai `font.ttf`、フォントのライセンスと通知を含めます。標準フォントは Git に保存し、`dist/font.ttf` にもコピーします。Release は ZIP、単独の `font.ttf`、両方の SHA256SUMS.txt を公開します。ゲーム素材、その他のローカルフォント、キャッシュ、設定、バックアップは除外されます。実行用 DLL は内蔵され、残りは Windows の構成要素です。

[build.ps1](build.ps1) · [package-release.ps1](package-release.ps1) · [Windows workflow](.github/workflows/windows-release.yml)

**moekuri-trans** は、未改変の日本語版 1.10 用の C++17/Win32 翻訳編集・抽出・EXE 作成ツールです。Windows 8 以降が必要です。翻訳とフォントを単一 EXE に埋め込みますが、元のゲームデータや音声ファイルも必要です。

1. `MoeKuriTools.exe` を起動し、UTF-8 の TXT・TSV・CSV・JSON 辞書を読み込みます。
2. 原文、訳文、項目ごとの幅を編集し、適用してから必要な形式で書き出します。空の訳文は原文を保持します。
3. 1 ページ 80 件を表示します。ページ番号を入力して移動ボタンまたは Enter を押せます。検索は辞書全体が対象です。
4. 原版 EXE と任意の TTF/OTF を選択します。**フォント最適化は既定でオン**で、必要な字形だけを埋め込みます。オフにすると完全なフォントを保持します。元のフォントファイルは変更しません。
5. 翻訳 EXE を作成し、元のリソースと同じディレクトリに置きます。EXE の展開で辞書、設定、埋め込みフォントを復元でき、フォントも書き出せます。

初回は Windows の表示言語を使用し、未対応の場合は英語になります。手動の選択は保存されます。UI の言語変更で辞書が自動翻訳されることはありません。更新後は再起動してください。

TXT/TSV の区切りは実際の TAB で、内部の改行・TAB・バックスラッシュは `\n`、`\t`、`\\` にします。CSV 列は `Original,Translation,WidthPixels` で、実際の複数行セルと標準の引用符規則を使います。JSON は項目ごとの幅を保存しません。既定値は倍率 100%、幅 620 px、最小横倍率 70% です。幅 0 は自動調整を無効にします。展開した最適化フォントにない文字を追加する場合は、完全なフォントを再選択してください。

VS 2022 の C++ デスクトップ開発と Windows SDK を用意し、`cmd /c build.cmd` でビルドします。Python 3 の `python tests/regression.py` で検証できます。完全な回帰検証には親ディレクトリの原版ゲームと、ローカルの `translation.txt`・`font.ttf` が必要です。Git はソース、検証スクリプト、辞書とライセンスを保持し、その他のローカルフォント、実行ファイル、設定、検証出力を除外します。[フォント通知](FONT-NOTICES.txt)と [OFL](FONT-LICENSE-OFL.txt)を参照してください。ソースコード独自のライセンスはまだ指定されていません。

現在の検証例はフォント最適化と可逆圧縮で 27.96 MiB から 8.72 MiB になりました。形式、ページ移動、字形、ロード時の検証は通過しています。実際の戦闘と全シナリオの目視確認は未完了で、抽出文字列も候補として確認が必要です。

## it-Italiano

### Esempio 1.10 e marcatore EXE

Importa catalogo apre `1.10trans.txt` o `1.10trans.csv`: **16.808 voci in cinese semplificato**, incluse le **13.084 voci di riferimento**, effetti del terreno e varianti di visualizzazione, righe e larghezza Unicode. Crediti e termini identici sono preservati. TXT: UTF-8; CSV: UTF-8 con BOM. All’avvio si usa `translation.txt`, altrimenti `1.10trans.txt`. La release minima include solo il TXT; il CSV si esporta dallo strumento.

Gli EXE generati hanno una sezione PE di sola lettura **`.mktrans`**: ASCII `moekuri_trans`, NUL, poi JSON UTF-8 con versione, SHA256 dell’EXE originale e del catalogo, numero di voci e modalità font. Usa un lettore PE o `MoeKuriTools.exe --inspect translated.exe`.

### Font e ringraziamenti

Il font predefinito è **小赖字体 / Xiaolai Font**. Grazie a LXGW per Xiaolai e a Nozomi Seto per SetoFont.

[Progetto: lxgw/kose-font](https://github.com/lxgw/kose-font) · [Licenza del font: SIL Open Font License 1.1](https://github.com/lxgw/kose-font/blob/master/OFL.txt) · [FONT-LICENSE-OFL.txt](FONT-LICENSE-OFL.txt) · [FONT-NOTICES.txt](FONT-NOTICES.txt)

### Font esterno e release

**[Font esterno]** cerca `font.otf`, poi `font.ttf`, accanto all’EXE del gioco; in assenza di un file valido usa il font di sistema. Rileva il nome reale e non incorpora font. Modalità esterna disattivata, ottimizzazione attivata per default. CLI: `--external-font 1`. Le descrizioni sono tradotte prima della divisione in righe.

Compila con `build.bat` o `build.ps1`; `-Package` controlla le dipendenze e crea il ZIP. Usa questa cartella come radice del repository GitHub. I tag `v*` pubblicano una Release. Il ZIP include EXE, README, 1.10trans.txt, il font Xiaolai predefinito `font.ttf` e i due file di licenza/avvisi. Il font predefinito è conservato in Git, copiato in `dist/font.ttf` e pubblicato separatamente con il ZIP e SHA256SUMS.txt, che verifica entrambi. Risorse del gioco, altri font locali, cache, impostazioni e backup sono esclusi. La DLL interna è incorporata; le altre appartengono a Windows.

[build.ps1](build.ps1) · [package-release.ps1](package-release.ps1) · [Windows workflow](.github/workflows/windows-release.yml)

**moekuri-trans** è uno strumento C++17/Win32 per modificare, estrarre e incorporare traduzioni nella versione giapponese 1.10 non modificata. Richiede Windows 8 o successivo. Produce un solo EXE con testi e font; restano necessari dati e audio originali.

1. Avvia `MoeKuriTools.exe` e importa TXT, TSV, CSV o JSON UTF-8.
2. Modifica originale, traduzione e larghezza, applica e poi esporta nel formato scelto. Una traduzione vuota conserva l’originale.
3. Sfoglia 80 voci per pagina oppure inserisci il numero e premi Vai / Invio. La ricerca copre tutte le voci.
4. Scegli l’EXE originale e un eventuale TTF/OTF. **L’ottimizzazione del font è attiva per impostazione predefinita** e incorpora solo i glifi necessari. Disattivala per includere il font completo. Il file originale non viene modificato.
5. Crea l’EXE tradotto accanto alle risorse del gioco. L’estrazione ripristina catalogo, impostazioni e font, esportabile separatamente.

La prima lingua segue Windows, altrimenti viene usato l’inglese; la scelta manuale viene salvata. Cambiare lingua dell’interfaccia non traduce il catalogo. Riavvia dopo un aggiornamento.

TXT/TSV usano vere tabulazioni tra colonne e `\n`, `\t`, `\\` nel testo. CSV usa `Original,Translation,WidthPixels`, celle realmente multilinea e virgolette standard. JSON non conserva le larghezze individuali. Valori iniziali: scala 100%, larghezza 620 px, minimo orizzontale 70%; larghezza 0 disattiva l’adattamento. Per aggiungere caratteri assenti nel font estratto e ridotto, seleziona di nuovo un font completo.

Installa VS 2022 con sviluppo desktop C++ e Windows SDK; compila con `cmd /c build.cmd`. Esegui `python tests/regression.py` con Python 3, preparando il gioco originale nella cartella superiore e `translation.txt`/`font.ttf` nel progetto. Git mantiene sorgenti, test, licenze e TXT, escludendo altri font locali, binari, impostazioni e risultati. Vedi [avvisi del font](FONT-NOTICES.txt) e [OFL](FONT-LICENSE-OFL.txt). Non è stata assegnata una licenza separata al codice sorgente.

Il caso verificato è passato da 27,96 a 8,72 MiB grazie ai glifi ridotti e alla compressione senza perdita. Formati, pagine, glifi e caricamento sono verificati; battaglie e storia completa richiedono ancora una verifica manuale. Le stringhe estratte sono candidate da esaminare.

## de-Deutsch

### 1.10-Importbeispiel und EXE-Markierung

Katalog importieren öffnet `1.10trans.txt` oder `1.10trans.csv`: **16.808 Einträge in vereinfachtem Chinesisch**, einschließlich aller **13.084 Referenzeinträge**, Geländeeffekte sowie Anzeige-, Zeilen- und Unicode-Breitenvarianten. Credits und gleich geschriebene Begriffe bleiben erhalten. TXT ist UTF-8, CSV UTF-8 mit BOM. Beim Start gilt `translation.txt`, sonst `1.10trans.txt`. Das minimale Release enthält nur TXT; CSV lässt sich im Werkzeug exportieren.

Erzeugte EXEs enthalten den schreibgeschützten PE-Abschnitt **`.mktrans`**: ASCII `moekuri_trans`, NUL, dann UTF-8-JSON mit Version, SHA256 von Original-EXE und Katalog, Eintragszahl und Schriftmodus. Prüfung mit PE-Viewer oder `MoeKuriTools.exe --inspect translated.exe`.

### Schrift und Danksagung

Die Standardschrift ist **小赖字体 / Xiaolai Font**. Vielen Dank an LXGW für Xiaolai und Nozomi Seto für SetoFont.

[Projekt: lxgw/kose-font](https://github.com/lxgw/kose-font) · [Schriftlizenz: SIL Open Font License 1.1](https://github.com/lxgw/kose-font/blob/master/OFL.txt) · [FONT-LICENSE-OFL.txt](FONT-LICENSE-OFL.txt) · [FONT-NOTICES.txt](FONT-NOTICES.txt)

### Externe Schrift und Releases

**[Externe Schrift]** versucht `font.otf`, danach `font.ttf`, neben der Spiel-EXE. Ohne gültige Datei wird die Systemschrift verwendet. Der echte Familienname wird erkannt; keine Schrift wird eingebettet. Extern standardmäßig aus, Optimierung an. CLI: `--external-font 1`. Beschreibungen werden vor dem Zeilenumbruch übersetzt.

Mit `build.bat` oder `build.ps1` bauen; `-Package` prüft Abhängigkeiten und erstellt das ZIP. Dieser Ordner ist die Wurzel des GitHub-Repositories. Tags `v*` veröffentlichen Releases. Das ZIP enthält EXE, README, 1.10trans.txt, die Standardschrift Xiaolai `font.ttf` und beide Schriftlizenz-/Hinweisdateien. Die Standardschrift bleibt in Git, wird nach `dist/font.ttf` kopiert und als eigene Release-Datei neben ZIP und SHA256SUMS.txt veröffentlicht. Die Prüfsummen decken ZIP und Schrift ab. Spielressourcen, andere lokale Schriften, Caches, Einstellungen und Sicherungen sind ausgeschlossen. Die interne DLL wird eingebettet; übrige DLLs gehören zu Windows.

[build.ps1](build.ps1) · [package-release.ps1](package-release.ps1) · [Windows workflow](.github/workflows/windows-release.yml)

**moekuri-trans** ist ein C++17/Win32-Werkzeug zum Bearbeiten, Extrahieren und Einbetten von Übersetzungen für die unveränderte japanische Version 1.10. Es benötigt Windows 8 oder neuer. Eine einzelne EXE enthält Texte und Schrift; die ursprünglichen Spieldaten und Audiodateien bleiben erforderlich.

1. Starten Sie `MoeKuriTools.exe` und importieren Sie UTF-8 TXT, TSV, CSV oder JSON.
2. Bearbeiten Sie Original, Übersetzung und Breite, übernehmen Sie die Änderung und exportieren Sie das gewünschte Format. Leere Übersetzungen behalten den Originaltext.
3. Jede Seite zeigt 80 Einträge. Geben Sie eine Seitennummer ein und wählen Sie Gehe zu / drücken Sie Enter. Die Suche umfasst den gesamten Katalog.
4. Wählen Sie die Original-EXE und optional TTF/OTF. **Schriftoptimierung ist standardmäßig aktiviert** und bettet nur benötigte Glyphen ein. Deaktivieren Sie sie für die vollständige Schrift. Die Quelldatei bleibt unverändert.
5. Erstellen Sie die übersetzte EXE neben den Originalressourcen. Entpacken stellt Katalog, Einstellungen und eingebettete Schrift wieder her; die Schrift kann separat exportiert werden.

Beim ersten Start gilt die Windows-Sprache, andernfalls Englisch; manuelle Auswahl wird gespeichert. Die Oberflächensprache übersetzt keine Katalogtexte. Nach einem Update neu starten.

TXT/TSV verwenden echte Tabulatoren als Trenner und `\n`, `\t`, `\\` im Text. CSV nutzt `Original,Translation,WidthPixels`, echte mehrzeilige Zellen und Standard-Anführungszeichen. JSON verliert individuelle Breiten. Standardwerte: 100% Größe, 620 px Breite, 70% Mindestbreite; Breite 0 deaktiviert die Anpassung. Für neue Zeichen nach dem Entpacken einer reduzierten Schrift wählen Sie erneut eine vollständige Schrift.

Installieren Sie VS 2022 mit C++-Desktopentwicklung und Windows SDK, dann `cmd /c build.cmd`. Python 3 führt `python tests/regression.py` aus. Dafür werden Originalspiel im übergeordneten Ordner sowie lokale `translation.txt`/`font.ttf` benötigt. Git behält Quelltexte, Tests, Lizenzen und TXT; andere lokale Schriften, Programme, Einstellungen und Ergebnisse werden ignoriert. Siehe [Schrifthinweise](FONT-NOTICES.txt) und [OFL](FONT-LICENSE-OFL.txt). Eine separate Quellcode-Lizenz wurde noch nicht festgelegt.

Der geprüfte Fall schrumpfte durch Schriftoptimierung und verlustfreie Kompression von 27,96 auf 8,72 MiB. Formate, Seitenwechsel, Glyphen und Ladeprüfungen sind verifiziert; echte Kämpfe und die komplette Geschichte benötigen noch manuelle Abnahme. Extrahierte Zeichenfolgen müssen geprüft werden.

## ru-Русский

### Пример 1.10 и метка EXE

Импорт словаря открывает `1.10trans.txt` или `1.10trans.csv`: **16 808 записей на упрощённом китайском**, включая все **13 084 исходную запись**, эффекты местности, подписи и варианты строк/ширины Unicode. Имена в титрах и одинаковые термины сохраняются. TXT — UTF-8, CSV — UTF-8 с BOM. При запуске используется `translation.txt`, иначе `1.10trans.txt`. Минимальный выпуск содержит только TXT; CSV экспортируется инструментом.

Созданный EXE содержит PE-секцию только для чтения **`.mktrans`**: ASCII `moekuri_trans`, NUL и JSON UTF-8 с версией, SHA256 оригинального EXE и словаря, количеством записей и режимом шрифта. Проверка: просмотрщик PE или `MoeKuriTools.exe --inspect translated.exe`.

### Шрифт и благодарности

Шрифт по умолчанию — **小赖字体 / Xiaolai Font**. Спасибо LXGW за Xiaolai и Nozomi Seto за исходный SetoFont.

[Проект: lxgw/kose-font](https://github.com/lxgw/kose-font) · [Лицензия шрифта: SIL Open Font License 1.1](https://github.com/lxgw/kose-font/blob/master/OFL.txt) · [FONT-LICENSE-OFL.txt](FONT-LICENSE-OFL.txt) · [FONT-NOTICES.txt](FONT-NOTICES.txt)

### Внешний шрифт и выпуски

**[Внешний шрифт]** проверяет `font.otf`, затем `font.ttf` рядом с EXE игры; без подходящего файла используется системный шрифт. Настоящее имя определяется из файла; шрифт не встраивается. Внешний режим выключен, оптимизация включена по умолчанию. CLI: `--external-font 1`. Описания переводятся до переноса строк.

Сборка: `build.bat` или `build.ps1`; `-Package` проверяет зависимости и создаёт ZIP. Эта папка должна быть корнем репозитория GitHub. Теги `v*` публикуют Release. ZIP содержит EXE, README, 1.10trans.txt, стандартный Xiaolai `font.ttf` и два файла лицензии/уведомлений. Стандартный шрифт хранится в Git, копируется в `dist/font.ttf` и публикуется отдельным файлом Release вместе с ZIP и SHA256SUMS.txt. Контрольные суммы проверяют оба файла. Ресурсы игры, другие локальные шрифты, кэш, настройки и резервные копии исключаются. Внутренняя DLL встроена; остальные являются компонентами Windows.

[build.ps1](build.ps1) · [package-release.ps1](package-release.ps1) · [Windows workflow](.github/workflows/windows-release.yml)

**moekuri-trans** — инструмент C++17/Win32 для редактирования, извлечения и упаковки переводов неизменённой японской версии 1.10. Требуется Windows 8 или новее. Тексты и шрифт входят в один EXE; исходные данные и аудио игры по-прежнему нужны.

1. Запустите `MoeKuriTools.exe` и импортируйте TXT, TSV, CSV или JSON в UTF-8.
2. Измените оригинал, перевод и ширину, примените изменения и экспортируйте выбранный формат. Пустой перевод сохраняет оригинал.
3. На странице 80 строк. Для перехода введите номер и нажмите Перейти / Enter. Поиск охватывает весь словарь.
4. Выберите оригинальный EXE и необязательный TTF/OTF. **Оптимизация шрифта включена по умолчанию**: сохраняются нужные глифы. Отключите её для полного шрифта. Исходный файл шрифта не изменяется.
5. Создайте переведённый EXE рядом с ресурсами игры. Распаковка восстанавливает словарь, настройки и встроенный шрифт; шрифт можно экспортировать отдельно.

Первый запуск использует язык Windows, иначе английский; ручной выбор запоминается. Смена языка интерфейса не переводит словарь. После обновления перезапустите инструмент.

TXT/TSV разделяют поля настоящей табуляцией; внутри текста используются `\n`, `\t`, `\\`. CSV содержит `Original,Translation,WidthPixels`, реальные многострочные ячейки и стандартные кавычки. JSON не сохраняет ширины строк. По умолчанию: масштаб 100%, ширина 620 px, минимум по горизонтали 70%; ширина 0 отключает подгонку. Если после распаковки сокращённого шрифта добавлены новые символы, выберите полный шрифт снова.

Установите VS 2022 с разработкой настольных приложений C++ и Windows SDK; сборка: `cmd /c build.cmd`. Проверки Python 3: `python tests/regression.py`; нужны оригинальная игра в родительской папке и локальные `translation.txt`/`font.ttf`. Git сохраняет исходники, скрипты, лицензии и TXT, исключая другие локальные шрифты, бинарные файлы, настройки и результаты. См. [уведомления о шрифте](FONT-NOTICES.txt) и [OFL](FONT-LICENSE-OFL.txt). Отдельная лицензия исходного кода пока не назначена.

Проверенный пример уменьшился с 27,96 до 8,72 MiB благодаря оптимизации глифов и сжатию без потерь. Форматы, переходы, глифы и загрузка проверены; реальные бои и полный сюжет требуют ручной проверки. Извлечённые строки являются кандидатами для просмотра.

## ar-العربية

### مثال 1.10 وعلامة EXE

افتح `1.10trans.txt` أو `1.10trans.csv` عبر استيراد القاموس. يحتوي المثال على **16,808 نصًا بالصينية المبسطة**، ويغطي **13,084 نصًا مرجعيًا**، إضافةً إلى تأثيرات التضاريس والتسميات واختلافات الأسطر وعرض Unicode. تُحفظ أسماء الاعتمادات والمصطلحات المتطابقة. TXT بترميز UTF-8 وCSV بترميز UTF-8 مع BOM. يبدأ بقراءة `translation.txt` ثم `1.10trans.txt` عند غيابه. الإصدار المصغّر يتضمن TXT فقط؛ يمكن تصدير CSV بالأداة.

يحتوي EXE الناتج على قسم PE للقراءة فقط **`.mktrans`**: ASCII `moekuri_trans` ثم NUL ثم JSON بترميز UTF-8 يسجّل الإصدار وSHA256 للعبة الأصلية والقاموس وعدد النصوص ووضع الخط. افحصه بعارض PE أو `MoeKuriTools.exe --inspect translated.exe`.

### الخط والشكر

الخط الافتراضي هو **小赖字体 / Xiaolai Font**. شكرًا لـ LXGW على Xiaolai ولـ Nozomi Seto على SetoFont الأصلي.

[المشروع: lxgw/kose-font](https://github.com/lxgw/kose-font) · [ترخيص الخط: SIL Open Font License 1.1](https://github.com/lxgw/kose-font/blob/master/OFL.txt) · [FONT-LICENSE-OFL.txt](FONT-LICENSE-OFL.txt) · [FONT-NOTICES.txt](FONT-NOTICES.txt)

### الخط الخارجي والإصدارات

يبحث خيار **[خط خارجي]** عن `font.otf` ثم `font.ttf` بجانب EXE اللعبة، ثم يستخدم خط النظام إذا لم يجد ملفًا صالحًا. يقرأ اسم الخط الحقيقي ولا يضمّن الخط. الوضع الخارجي معطّل افتراضيًا وتحسين الخط مفعّل. CLI: `--external-font 1`. تُترجم الأوصاف قبل تقسيم الأسطر.

للبناء استخدم `build.bat` أو `build.ps1`؛ يفحص `-Package` الاعتماديات ويُنشئ ZIP. اجعل هذا المجلد جذر مستودع GitHub. تنشر وسوم `v*` إصدار Release. يحتوي ZIP على EXE وREADME و1.10trans.txt وخط Xiaolai الافتراضي `font.ttf` وملفي الترخيص والإشعارات. يُحفظ الخط الافتراضي في Git ويُنسخ إلى `dist/font.ttf` ويُنشر منفصلًا في Release مع ZIP وSHA256SUMS.txt للتحقق من الملفين. تُستبعد ملفات اللعبة والخطوط المحلية الأخرى والذاكرة المؤقتة والإعدادات والنسخ الاحتياطية. DLL الداخلي مضمّن، والبقية مكونات Windows.

[build.ps1](build.ps1) · [package-release.ps1](package-release.ps1) · [Windows workflow](.github/workflows/windows-release.yml)

**moekuri-trans** أداة C++17/Win32 لتحرير الترجمات واستخراجها وتضمينها في النسخة اليابانية 1.10 غير المعدّلة. تتطلب Windows 8 أو أحدث. تضمّن النصوص والخط في ملف EXE واحد، مع بقاء الحاجة إلى بيانات اللعبة وملفات الصوت الأصلية.

1. شغّل `MoeKuriTools.exe` واستورد TXT أو TSV أو CSV أو JSON بترميز UTF-8.
2. حرّر النص الأصلي والترجمة والعرض، ثم طبّق التغيير وصدّر بالصيغة المطلوبة. الترجمة الفارغة تُبقي النص الأصلي.
3. تُعرض 80 خانة في كل صفحة. أدخل رقم الصفحة واضغط انتقال أو Enter. يشمل البحث القاموس بالكامل.
4. اختر EXE الأصلي وخط TTF/OTF اختياريًا. **تحسين الخط مفعّل افتراضيًا** لتضمين أشكال الحروف المطلوبة فقط. ألغِ تحديده لتضمين الخط كاملًا. لا يتغيّر ملف الخط الأصلي.
5. أنشئ EXE المترجم وضعه بجوار موارد اللعبة الأصلية. يعيد فك EXE القاموس والإعدادات والخط المضمّن، ويمكن تصدير الخط منفصلًا.

تتبع الواجهة لغة Windows في التشغيل الأول، وتستخدم الإنجليزية عند عدم دعم اللغة. يُحفظ الاختيار اليدوي؛ تغيير لغة الواجهة لا يترجم القاموس. أعد التشغيل بعد التحديث.

يفصل TXT/TSV الأعمدة بمحرف TAB فعلي؛ تُكتب المحارف الداخلية بصيغ `\n` و`\t` و`\\`. أعمدة CSV هي `Original,Translation,WidthPixels`، مع خلايا متعددة الأسطر وقواعد الاقتباس القياسية. لا يحفظ JSON عرض كل خانة. القيم الافتراضية: مقياس 100%، عرض 620 px، وحد أدنى أفقي 70%؛ العرض 0 يعطّل الملاءمة. عند إضافة حروف بعد استخراج خط مختصر، اختر خطًا كاملًا مجددًا. تحتفظ الأداة بالخط كاملًا عند اكتشاف نصوص ترجمة ذات تشكيل معقّد تدعمه آلية الحماية الحالية.

ثبّت VS 2022 مع تطوير تطبيقات سطح المكتب C++ وWindows SDK، ثم نفّذ `cmd /c build.cmd`. للاختبار باستخدام Python 3: `python tests/regression.py`، مع اللعبة الأصلية في المجلد الأب وملفي `translation.txt` و`font.ttf` محليًا. يحتفظ Git بالمصادر والاختبارات والتراخيص وقاموس TXT، ويستبعد الخطوط المحلية الأخرى والبرامج والإعدادات والنتائج. راجع [إشعارات الخط](FONT-NOTICES.txt) و[OFL](FONT-LICENSE-OFL.txt). لم يُحدّد ترخيص منفصل للشيفرة المصدرية بعد.

انخفض المثال المختبَر من 27.96 إلى 8.72 MiB عبر تحسين الخط والضغط دون فقدان. تم التحقق من الصيغ والصفحات والحروف والتحميل؛ ما زالت المعارك والقصة الكاملة بحاجة إلى اختبار يدوي، والنصوص المستخرجة بحاجة إلى مراجعة.

## es-Español

### Ejemplo 1.10 y marca EXE

Importar catálogo abre `1.10trans.txt` o `1.10trans.csv`: **16.808 entradas en chino simplificado**, incluidas las **13.084 de referencia**, efectos del terreno y variantes de etiquetas, líneas y anchura Unicode. Se conservan créditos y términos idénticos. TXT usa UTF-8; CSV, UTF-8 con BOM. Al iniciar se carga `translation.txt`, o `1.10trans.txt` si falta. La versión mínima incluye solo TXT; el CSV se exporta desde la herramienta.

Los EXE generados contienen una sección PE de solo lectura **`.mktrans`**: ASCII `moekuri_trans`, NUL y JSON UTF-8 con versión, SHA256 del EXE original y del catálogo, número de entradas y modo de fuente. Usa un visor PE o `MoeKuriTools.exe --inspect translated.exe`.

### Fuente y agradecimientos

La fuente predeterminada es **小赖字体 / Xiaolai Font**. Gracias a LXGW por Xiaolai y a Nozomi Seto por SetoFont.

[Proyecto: lxgw/kose-font](https://github.com/lxgw/kose-font) · [Licencia de la fuente: SIL Open Font License 1.1](https://github.com/lxgw/kose-font/blob/master/OFL.txt) · [FONT-LICENSE-OFL.txt](FONT-LICENSE-OFL.txt) · [FONT-NOTICES.txt](FONT-NOTICES.txt)

### Fuente externa y versiones

**[Fuente externa]** prueba `font.otf`, después `font.ttf`, junto al EXE del juego; si no hay archivo válido usa la fuente del sistema. Detecta el nombre real y no incrusta fuentes. Modo externo desactivado y optimización activada por defecto. CLI: `--external-font 1`. Las descripciones se traducen antes de dividir las líneas.

Compila con `build.bat` o `build.ps1`; `-Package` comprueba dependencias y genera el ZIP. Esta carpeta debe ser la raíz del repositorio GitHub. Los tags `v*` publican una Release. El ZIP incluye EXE, README, 1.10trans.txt, la fuente Xiaolai predeterminada `font.ttf` y los dos archivos de licencia/avisos. La fuente predeterminada se conserva en Git, se copia a `dist/font.ttf` y se publica por separado junto al ZIP y SHA256SUMS.txt, que verifica ambos archivos. Se excluyen recursos del juego, otras fuentes locales, cachés, ajustes y copias de seguridad. La DLL interna está incrustada; las restantes son componentes de Windows.

[build.ps1](build.ps1) · [package-release.ps1](package-release.ps1) · [Windows workflow](.github/workflows/windows-release.yml)

**moekuri-trans** es una herramienta C++17/Win32 para editar, extraer y empaquetar traducciones de la versión japonesa 1.10 sin modificar. Requiere Windows 8 o posterior. Genera un único EXE con textos y fuente; siguen siendo necesarios los datos y el audio originales.

1. Inicia `MoeKuriTools.exe` e importa TXT, TSV, CSV o JSON UTF-8.
2. Edita original, traducción y ancho, aplica y exporta el formato elegido. Una traducción vacía conserva el original.
3. Se muestran 80 entradas por página. Introduce un número y pulsa Ir / Enter. La búsqueda incluye todo el catálogo.
4. Elige el EXE original y, opcionalmente, un TTF/OTF. **La optimización de fuente está activada por defecto** y solo incorpora los glifos necesarios. Desactívala para conservar la fuente completa. El archivo original no cambia.
5. Genera el EXE traducido junto a los recursos originales. Desempaquetar recupera el catálogo, los ajustes y la fuente incluida, que también puede exportarse.

El primer inicio usa el idioma de Windows, o inglés si no está disponible; la elección manual se guarda. Cambiar el idioma de la interfaz no traduce el catálogo. Reinicia después de actualizar.

TXT/TSV separan columnas con tabuladores reales y usan `\n`, `\t`, `\\` dentro del texto. CSV utiliza `Original,Translation,WidthPixels`, celdas realmente multilínea y comillas estándar. JSON pierde los anchos individuales. Valores iniciales: escala 100%, ancho 620 px, mínimo horizontal 70%; ancho 0 desactiva el ajuste. Si añades caracteres tras extraer una fuente reducida, vuelve a elegir una fuente completa.

Instala VS 2022 con desarrollo de escritorio C++ y Windows SDK; compila con `cmd /c build.cmd`. Ejecuta `python tests/regression.py` con Python 3, preparando el juego original en la carpeta superior y `translation.txt`/`font.ttf` localmente. Git conserva código, scripts, licencias y TXT; ignora otras fuentes locales, binarios, ajustes y resultados. Consulta [avisos de fuente](FONT-NOTICES.txt) y [OFL](FONT-LICENSE-OFL.txt). Todavía no se ha asignado una licencia independiente al código fuente.

El caso probado bajó de 27,96 a 8,72 MiB mediante reducción de glifos y compresión sin pérdida. Se verificaron formatos, páginas, glifos y carga; las batallas reales y la historia completa aún requieren revisión manual. Los textos extraídos son candidatos que deben revisarse.
