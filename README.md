# moekuri-trans

C++17 · Win32 x86 · Windows 8+ · UTF-8 catalogs · MoeKuri 2 JP 1.10

[简体中文](#zh-简体中文) · [English](#en-english) · [한국어](#ko-한국어) · [Français](#fr-français) · [日本語](#ja-日本語) · [Italiano](#it-italiano) · [Deutsch](#de-deutsch) · [Русский](#ru-русский) · [العربية](#ar-العربية) · [Español](#es-español)

```powershell
# Build / minimal release (Windows, VS 2022 C++ + SDK)
.\build.ps1 -Package

# Extract/export EXE text only (default); add --omit-dxa 0 for full text
.\MoeKuriTools.exe --extract original.exe originals.csv
.\MoeKuriTools.exe --convert 1.10trans.txt translation.csv

# Edit resources after unpacking, then rebuild a v4 archive outside the folder
.\MoeKuriTools.exe --dxa-unpack data\csv.dxa unpacked-csv
.\MoeKuriTools.exe --dxa-pack unpacked-csv new-csv.dxa

# Full font archive; GUI generation can subset it using the catalog
.\MoeKuriTools.exe --font-archive font.ttf font.dxa

# Translation EXE + optimized font.dxa (place both with original game assets)
.\MoeKuriTools.exe --pack original.exe 1.10trans_omidxa.txt moekuri_cn.exe --font font.ttf --font-dxa 1

# Single EXE with embedded font
.\MoeKuriTools.exe --pack original.exe 1.10trans.txt moekuri_cn.exe --omit-dxa 0 --font font.ttf --external-font 0

# CI-compatible DXA/CSV checks; --native additionally tests the local game
python tests/dxa.py
python tests/dxa.py --native
```

Release ZIP: `MoeKuriTools.exe`, `README.md`, `1.10trans.txt`, `1.10trans_omidxa.txt`, `font.ttf`, `FONT-LICENSE-OFL.txt`, `FONT-NOTICES.txt`.
CLI `--external-font 1` only selects lookup of existing external files; `--font-dxa 1 --font PATH` exports the archive. The GUI also exports the selected loose font.

Release assets: ZIP, `font.ttf`, `SHA256SUMS.txt`. No proprietary game resources.

## zh-简体中文

用于《萌库里2》日文 1.10 的翻译编辑与 modding（修改、资源提取、解包和回包）。界面首次打开默认英文，之后记住手动选择的语言。工具语言用于默认输出名：`moekuri_cn.exe`、`moekuri_en.exe`、`moekuri_jp.exe`、`moekuri_kr.exe` 等；可在保存对话框改名为 `moekuri_us.exe`、`moekuri_eu.exe` 或其他名称。选择界面语言不会自动翻译词库。

点击“导入词库 / EXE / DXA”：TXT、CSV、TSV、JSON 导入词库；原版 EXE 提取原文；翻译 EXE 读取内嵌词库和参数；DXA 解包到选择的目录。选中上方词条后，在下方编辑对应译文；双击译文列可进入编辑。支持搜索、未翻译筛选、80 条分页与页码跳转。

“屏蔽 DXA 翻译”默认开启：提取、导出、生成只保留 EXE 文本，同时存在于 EXE/DXA 的文本也排除。完整词库留在编辑器中；关闭此选项可翻译归档里的图鉴、剧情、技能及其他显示字段。`1.10trans_omidxa.txt` 是 1,805 条 EXE 文本导入样例，`1.10trans.txt` 是完整样例。

“外置字体”和“字体优化”默认开启。请选择实际 TTF/OTF 文件；字体名称从文件识别，不依赖翻译者电脑上的字体列表。生成 EXE 时一并导出裁剪后的字体。开启 `[font.dxa]` 会将字体压缩到 EXE 同目录的 `font.dxa`，运行时解压注册并将游戏字体名称重定向到该字体。优先级是 `font.dxa`（该模式开启时）→ `font.otf` → `font.ttf` → 系统字体，缺失或无效文件会回退。关闭外置字体可将字体嵌入单个 EXE；外置模式需连同字体文件一起分发。

“DXA 解包 / DXA 打包”用于修改资源后重新打包，兼容本游戏的 DXArchive v4、CP932 文件名和游戏密钥。资源内容按字节保留，原生 UTF-16 文件不会被强制改成 UTF-8。编辑词库与导出文本统一 UTF-8。解包不覆盖已有文件，拒绝越界路径和符号链接；回包输出必须位于输入目录之外。此功能不生成 EXE 的反编译 C++ 源码。

CSV 列为 `Original,Translated,WidthPixels,Width%,Escapes`：`WidthPixels=0` 使用默认宽度；`Width%=100` 保持本条比例，范围 25–300；与全局比例相乘。换行、回车、制表符显示为 `<lf>`、`<cr>`、`<tab>`，字面 `<` 显示为 `<lt>`；保留 `Escapes=moekuri-tags-v1`，不要删除标签或 `%s/%d` 参数。旧 CSV 和三列 TXT 仍可导入；TXT 可用第四列存本条缩放。JSON 仅保留原文/译文，不保存宽度或缩放。

剧情控制命令及 `@ブラリボン` 之类标识符保留原文；显示名称可翻译。在完整模式下，剧情和图鉴介绍先在读取阶段翻译，避免逐字显示日文后才替换。图片中的日文不属于文本翻译功能。生成 EXE 含只读 `.mktrans` 区和 `moekuri_trans` 特征标记。

安装 Visual Studio 2022 的 C++ 桌面开发与 Windows SDK，运行 `build.bat` 或 `build.ps1 -Package`。内部运行时 DLL 已嵌入工具与生成的 EXE，其他 DLL 使用 Windows 系统组件。GitHub Actions 在 Windows 编译，`v*` 标签自动发布 Release，只打包明确列出的文件；包括默认 `font.ttf`，不包含游戏资源、测试夹具或备份。

默认字体为 **小赖字体 / Xiaolai Font**。感谢 [lxgw/kose-font](https://github.com/lxgw/kose-font)；字体采用 [SIL Open Font License 1.1](FONT-LICENSE-OFL.txt)，详见 [字体致谢与声明](FONT-NOTICES.txt)。DXA 格式参考 [DxLib for Linux](https://github.com/dragoon2014/dxlib-for-linux) 与本地 DXArchive 参考实现；本项目未复制该仓库源码。项目 C++ 源码尚未指定独立开源许可证。

## en-english

Translation editing and modding for the original Japanese MoeKuri 2 1.10. The first launch uses English, then remembers your selected UI language. Default output names follow that language: `moekuri_cn.exe`, `moekuri_en.exe`, `moekuri_jp.exe`, `moekuri_kr.exe`, etc. You can rename them to `moekuri_us.exe`, `moekuri_eu.exe` or another name. Choosing a UI language does not translate the catalog.

Import accepts TXT/CSV/TSV/JSON catalogs, extracts an original EXE, recovers a translated EXE, or unpacks DXA to a chosen folder. Select a row in the full-width list and edit its translation below; double-click a translation to focus the editor. Search, untranslated-only filtering, bounded 80-row pages and page jumps are supported.

Exclude DXA translation is on by default. Extraction, export and EXE generation retain only EXE text, excluding keys shared with DXA. Disable it for encyclopedia, story, skill and other archive text. The editor retains the full catalog. `1.10trans_omidxa.txt` is the 1,805-row EXE-only sample; `1.10trans.txt` is the full sample.

External fonts and subsetting default on. Select an actual TTF/OTF file; its family name is detected from the file. Generation exports the selected/subset font beside the game. Enable `[font.dxa]` to compress it into `font.dxa`, load/register it at startup and redirect game font requests to its family. Priority: enabled `font.dxa` → `font.otf` → `font.ttf` → system font. Invalid/missing files fall back. Disable external fonts for a single EXE with an embedded font; distribute external font files with external-mode games.

DXA unpack/repack preserves resource bytes using this game’s v4 format, CP932 filenames and key. Native UTF-16 resources remain UTF-16; editable catalogs use UTF-8. Extraction refuses existing output files, unsafe paths and symlinks. Pack outside the input folder. This archive tool does not reconstruct C++ source from an EXE.

CSV columns: `Original,Translated,WidthPixels,Width%,Escapes`. WidthPixels 0 inherits the default; Width% is per-row scale, 25–300, multiplied by the global scale. Control characters appear as `<cr>`, `<lf>`, `<tab>`; literal `<` becomes `<lt>`. Retain `Escapes=moekuri-tags-v1`, tags and printf parameters. Legacy CSV/three-column TXT remain readable; optional fourth TXT column stores scale. JSON stores text only.

Script commands and standalone @ identifiers remain unchanged. Full mode translates story/description reads before rendering. Image text is outside text replacement. Generated EXEs carry a read-only `.mktrans` section and `moekuri_trans` signature.

Build with VS 2022 C++ desktop tools and Windows SDK using `build.bat` or `build.ps1 -Package`. The runtime DLL is embedded; other DLLs are Windows components. Windows GitHub Actions publishes v-prefixed tags with a strict file allowlist, including default font.ttf, excluding game data, fixtures and backups.

Thanks to [lxgw/kose-font](https://github.com/lxgw/kose-font) for the default **Xiaolai Font**, under [SIL OFL 1.1](FONT-LICENSE-OFL.txt); see [font notices](FONT-NOTICES.txt). DXA format references: [DxLib for Linux](https://github.com/dragoon2014/dxlib-for-linux) and local DXArchive references. No source from that repository is copied. No separate license has yet been assigned to this project’s C++ source.

## ko-한국어

일본어 원본 MoeKuri 2 1.10 번역 편집 및 모딩 도구입니다. 처음에는 영어 UI를 사용하고 이후 선택을 기억합니다. UI 언어에 따라 `moekuri_cn/en/jp/kr.exe` 등의 기본 이름이 정해집니다. 저장할 때 이름을 변경할 수 있으며 UI 선택만으로 번역되지는 않습니다.

통합 가져오기로 TXT/CSV/TSV/JSON, 원본 EXE 추출, 번역 EXE 복원, DXA 압축 풀기를 지원합니다. 위 목록에서 선택하면 아래 번역 편집기가 대응합니다. 번역을 두 번 클릭하여 편집합니다. 검색, 미번역 필터, 80개씩 페이지 이동을 제공합니다.

DXA 번역 제외는 기본 켜짐입니다. 추출/내보내기/생성은 EXE 전용 텍스트만 보존하며 DXA와 공유된 항목도 제외합니다. 끄면 도감/대사/기술 등의 전체 번역을 사용합니다. `1.10trans_omidxa.txt`는 EXE 전용 1,805개 예제이고 `1.10trans.txt`는 전체 예제입니다.

외부 글꼴과 글리프 최적화는 기본 켜짐입니다. TTF/OTF 파일을 선택하면 실제 이름을 읽어 글꼴도 내보냅니다. `[font.dxa]`는 글꼴을 압축하고 게임의 글꼴 요청을 등록된 글꼴로 리디렉션합니다. 우선순위: 활성 font.dxa → font.otf → font.ttf → 시스템 글꼴. 오류 시 다음 글꼴로 전환합니다. 외부 모드를 끄면 글꼴을 단일 EXE에 포함합니다.

DXA v4/CP932/게임 키로 압축을 풀고 수정한 폴더를 다시 압축합니다. 리소스 바이트와 UTF-16은 보존하고 편집 사전은 UTF-8입니다. 기존 출력 파일, 위험 경로, 심볼릭 링크는 거부합니다. 입력 폴더 밖에 압축 파일을 저장합니다. EXE에서 C++ 소스를 복원하지는 않습니다.

CSV: `Original,Translated,WidthPixels,Width%,Escapes`. WidthPixels 0은 기본값, Width%는 항목 배율(25–300)입니다. `<cr>/<lf>/<tab>/<lt>` 태그와 `moekuri-tags-v1`, printf 인수를 보존하세요. 기존 CSV/TXT도 읽으며 TXT 네 번째 열은 배율입니다. JSON에는 레이아웃이 없습니다. @ 식별자와 명령은 보존됩니다.

VS 2022 C++ 및 Windows SDK로 `build.bat` 또는 `build.ps1 -Package`를 실행합니다. DLL 런타임은 포함됩니다. Windows Actions는 v* 태그 Release에 허용된 파일과 font.ttf만 배포합니다. 원본 게임 데이터는 별도로 필요합니다.

기본 **Xiaolai Font**를 제공한 [lxgw/kose-font](https://github.com/lxgw/kose-font)에 감사드립니다. [OFL 1.1](FONT-LICENSE-OFL.txt), [글꼴 고지](FONT-NOTICES.txt). DXA 형식 참고: [DxLib for Linux](https://github.com/dragoon2014/dxlib-for-linux). 프로젝트 C++ 소스에는 별도 라이선스가 아직 없습니다.

## fr-français

Éditeur de traduction et outil de modding pour MoeKuri 2 japonais 1.10. Premier lancement en anglais, puis langue mémorisée. La langue de l’interface détermine le nom proposé : moekuri_cn/en/jp/kr/fr.exe, etc. Le nom reste modifiable ; changer de langue ne traduit pas le catalogue.

Import unifié : TXT/CSV/TSV/JSON, extraction de l’EXE original, récupération d’un EXE traduit ou extraction DXA. Sélectionnez une ligne puis éditez la traduction en dessous ; double-cliquez pour activer l’éditeur. Recherche, filtre des entrées non traduites, pages de 80 lignes et saut de page.

Exclusion DXA activée par défaut : extraction/export/génération limités aux textes EXE, hors clés partagées avec DXA. Désactivez pour les descriptions, dialogues et compétences. Exemple EXE seul : 1.10trans_omidxa.txt (1 805 lignes) ; exemple complet : 1.10trans.txt.

Police externe et optimisation activées par défaut. Choisissez un fichier TTF/OTF ; son vrai nom est détecté et la police exportée. L’option [font.dxa] compresse la police, l’enregistre au démarrage et redirige les demandes du jeu. Priorité : font.dxa activé → font.otf → font.ttf → police système ; repli si absent/invalide. Désactivez la police externe pour tout intégrer dans un seul EXE.

DXA v4 utilise les noms CP932 et la clé du jeu. Extraction puis modification et reconstruction, avec octets et UTF-16 d’origine conservés ; catalogues en UTF-8. Refus des fichiers existants, chemins dangereux et liens symboliques ; archive hors dossier source. Ne reconstitue pas le code C++ d’un EXE.

CSV : Original,Translated,WidthPixels,Width%,Escapes. WidthPixels=0 hérite du défaut ; Width% est l’échelle par entrée (25–300), multipliée par l’échelle globale. Conservez <cr>, <lf>, <tab>, <lt>, moekuri-tags-v1 et les paramètres printf. Anciens CSV/TXT compatibles ; quatrième colonne TXT pour l’échelle. JSON conserve seulement les textes. Commandes et identifiants @ préservés.

Compilez avec VS 2022 C++/Windows SDK : build.bat ou build.ps1 -Package. DLL interne intégrée. Actions Windows publie les tags v* avec une liste stricte de fichiers dont font.ttf ; ressources originales toujours nécessaires.

Merci à [lxgw/kose-font](https://github.com/lxgw/kose-font) pour **Xiaolai Font**, sous [OFL 1.1](FONT-LICENSE-OFL.txt), [mentions](FONT-NOTICES.txt). Référence DXA : [DxLib for Linux](https://github.com/dragoon2014/dxlib-for-linux). Le code C++ du projet n’a pas encore de licence distincte.

## ja-日本語

日本語版 MoeKuri 2 1.10 用の翻訳編集・modding ツールです。初回 UI は英語、以降は選択を保存します。UI 言語に応じて moekuri_cn/en/jp/kr.exe などを提案します。保存名は変更可能です。UI 選択による自動翻訳はありません。

統合読み込みで TXT/CSV/TSV/JSON、原版 EXE の抽出、翻訳 EXE の復元、DXA 展開を選べます。上の一覧を選択すると下に訳文が表示され、ダブルクリックで編集できます。検索、未翻訳フィルター、80 件のページ表示・移動に対応します。

DXA 翻訳除外は既定で有効です。抽出・出力・生成は EXE のみで、DXA と共通の項目も除外します。無効にすると図鑑、会話、技などの全体翻訳を使用します。1.10trans_omidxa.txt は EXE 専用 1,805 件、1.10trans.txt は全体の例です。

外部フォントと最適化は既定で有効です。TTF/OTF ファイルから実際の名前を取得し、生成時にフォントも出力します。[font.dxa] はフォントを圧縮し、起動時に登録してゲームのフォントを転送します。優先順位：有効な font.dxa → font.otf → font.ttf → システム。欠落・不正時は次へ移ります。外部モードを無効にすると単一 EXE に埋め込めます。

本ゲームの DXA v4、CP932 名、キーで展開・再作成できます。リソースのバイト列と UTF-16 は保持し、編集辞書は UTF-8 です。既存出力・危険なパス・シンボリックリンクを拒否します。出力は入力フォルダー外に保存してください。EXE の C++ ソースを復元する機能ではありません。

CSV 列：Original,Translated,WidthPixels,Width%,Escapes。WidthPixels=0 は既定、Width% は項目倍率（25–300）で全体倍率と乗算されます。<cr>/<lf>/<tab>/<lt>、moekuri-tags-v1 と printf 引数を保持してください。旧 CSV/TXT も読み込め、TXT 第 4 列は倍率です。JSON は文字列のみです。コマンドと @ 識別子は保持します。

VS 2022 C++ と Windows SDK を使用し build.bat または build.ps1 -Package でビルドします。実行時 DLL は内蔵。Windows Actions は v* タグを限定ファイルと font.ttf で Release 公開します。原版リソースは別途必要です。

既定の **小頼フォント / Xiaolai Font**：[lxgw/kose-font](https://github.com/lxgw/kose-font) に感謝します。[OFL 1.1](FONT-LICENSE-OFL.txt)、[通知](FONT-NOTICES.txt)。DXA 形式参考：[DxLib for Linux](https://github.com/dragoon2014/dxlib-for-linux)。本 C++ ソースに独立ライセンスはまだ指定されていません。

## it-italiano

Editor di traduzioni e modding per MoeKuri 2 giapponese 1.10. Primo avvio in inglese, poi lingua salvata. Nomi proposti dalla lingua UI: moekuri_cn/en/jp/kr/it.exe, modificabili al salvataggio. La scelta della lingua non traduce il catalogo.

Importa TXT/CSV/TSV/JSON, estrai l’EXE originale, recupera un EXE tradotto o estrai DXA. Seleziona una riga e modifica la traduzione sotto; doppio clic per attivare l’editor. Ricerca, filtro non tradotti, 80 righe per pagina e salto pagina.

Esclusione DXA attiva per impostazione predefinita: estrazione/esportazione/generazione solo EXE, escluse chiavi condivise. Disattivala per descrizioni/dialoghi/abilità. 1.10trans_omidxa.txt contiene 1.805 voci EXE; 1.10trans.txt è completo.

Font esterno e ottimizzazione attivi per default. Scegli TTF/OTF; nome rilevato dal file ed esportazione insieme all’EXE. [font.dxa] comprime il font, lo registra e reindirizza le richieste del gioco. Priorità: font.dxa attivato → font.otf → font.ttf → sistema. File invalidi/assenti fanno passare al successivo. Disattiva il font esterno per incorporarlo in un solo EXE.

Archivio DXA v4 con nomi CP932 e chiave del gioco. Estrazione, modifica e ricostruzione preservano byte/UTF-16; cataloghi UTF-8. Rifiuta sovrascritture, percorsi pericolosi e symlink. Salva fuori dalla cartella sorgente. Non ricostruisce sorgenti C++ da EXE.

CSV: Original,Translated,WidthPixels,Width%,Escapes. WidthPixels=0 eredita il valore predefinito; Width% è scala per voce (25–300), moltiplicata per la globale. Conserva <cr>/<lf>/<tab>/<lt>, moekuri-tags-v1 e parametri printf. Compatibili vecchi CSV/TXT; quarta colonna TXT per la scala. JSON solo testi. Comandi e identificatori @ conservati.

VS 2022 C++ e Windows SDK: build.bat o build.ps1 -Package. DLL interna incorporata. Actions Windows pubblica tag v* con una lista minima, incluso font.ttf. Servono i dati originali del gioco.

Grazie a [lxgw/kose-font](https://github.com/lxgw/kose-font) per **Xiaolai Font**, [OFL 1.1](FONT-LICENSE-OFL.txt), [avvisi](FONT-NOTICES.txt). Riferimento DXA: [DxLib for Linux](https://github.com/dragoon2014/dxlib-for-linux). Il C++ del progetto non ha ancora una licenza separata.

## de-deutsch

Übersetzungseditor und Modding-Werkzeug für die japanische Version MoeKuri 2 1.10. Erststart auf Englisch; danach wird die UI-Sprache gespeichert. Vorgeschlagene Namen: moekuri_cn/en/jp/kr/de.exe, beim Speichern änderbar. Die UI-Sprache übersetzt den Katalog nicht automatisch.

Importiert TXT/CSV/TSV/JSON, extrahiert Original-EXEs, liest übersetzte EXEs und entpackt DXA. Zeile oben auswählen und Übersetzung unten bearbeiten; Doppelklick aktiviert den Editor. Suche, Unübersetzt-Filter, 80 Zeilen pro Seite und Seitensprung.

DXA-Ausschluss ist standardmäßig aktiv: Extraktion/Export/Generierung enthalten nur EXE-Texte ohne gemeinsame DXA-Schlüssel. Für Beschreibungen/Dialoge/Fähigkeiten deaktivieren. 1.10trans_omidxa.txt: 1.805 EXE-Einträge; 1.10trans.txt: vollständiges Beispiel.

Externe Schrift und Optimierung sind standardmäßig aktiv. TTF/OTF-Datei wählen; Familienname wird erkannt und Schrift mit exportiert. [font.dxa] komprimiert, registriert und leitet Spielschriftanfragen um. Reihenfolge: aktiviertes font.dxa → font.otf → font.ttf → System. Fehlende/ungültige Dateien werden übersprungen. Extern deaktivieren, um alles in eine EXE einzubetten.

DXA v4 mit CP932-Dateinamen und Spielschlüssel: entpacken, bearbeiten, neu packen. Ressourcenbytes/UTF-16 bleiben erhalten, Kataloge sind UTF-8. Vorhandene Ausgabedateien, unsichere Pfade und Symlinks werden abgelehnt. Archiv außerhalb des Quellordners speichern. Kein Wiederherstellen von C++-Quelltext aus EXEs.

CSV: Original,Translated,WidthPixels,Width%,Escapes. WidthPixels=0 nutzt den Standard, Width%=25–300 die Zeilenskalierung multipliziert mit globaler Skalierung. <cr>/<lf>/<tab>/<lt>, moekuri-tags-v1 und printf-Parameter erhalten. Alte CSV/TXT lesbar; vierte TXT-Spalte für Skalierung. JSON nur Texte. Befehle und @-Kennungen bleiben erhalten.

VS 2022 C++/Windows SDK: build.bat oder build.ps1 -Package. Interne DLL eingebettet. Windows Actions veröffentlicht v*-Tags mit strikter Dateiliste samt font.ttf. Originale Spieldaten weiterhin nötig.

Danke an [lxgw/kose-font](https://github.com/lxgw/kose-font) für **Xiaolai Font**, [OFL 1.1](FONT-LICENSE-OFL.txt), [Hinweise](FONT-NOTICES.txt). DXA-Referenz: [DxLib for Linux](https://github.com/dragoon2014/dxlib-for-linux). Noch keine separate Lizenz für den Projekt-C++-Quelltext.

## ru-русский

Редактор перевода и инструмент моддинга японской MoeKuri 2 1.10. Первый запуск на английском, затем язык интерфейса сохраняется. Имена по языку: moekuri_cn/en/jp/kr/ru.exe; при сохранении можно изменить. Выбор языка не переводит словарь.

Общий импорт TXT/CSV/TSV/JSON, извлечение оригинального EXE, чтение переведённого EXE, распаковка DXA. Выберите строку сверху и редактируйте перевод снизу; двойной щелчок включает редактор. Поиск, фильтр непереведённых, страницы по 80 строк и переход.

Исключение DXA включено: извлечение/экспорт/сборка оставляют только EXE-тексты без общих с DXA ключей. Отключите для описаний, диалогов и навыков. 1.10trans_omidxa.txt — 1 805 EXE-записей; 1.10trans.txt — полный пример.

Внешний шрифт и оптимизация включены. Выберите TTF/OTF: имя определяется из файла, шрифт экспортируется рядом с EXE. [font.dxa] сжимает шрифт, регистрирует его и перенаправляет запросы игры. Приоритет: включённый font.dxa → font.otf → font.ttf → системный. Отсутствующий/повреждённый файл пропускается. Отключите внешнюю загрузку для единого EXE со шрифтом.

DXA v4, имена CP932, ключ игры: распаковка, правка, упаковка. Байты и UTF-16 ресурсов сохраняются; словарь UTF-8. Существующие файлы, опасные пути и ссылки запрещены. Архив выводится вне исходной папки. Восстановление исходного C++ из EXE не выполняется.

CSV: Original,Translated,WidthPixels,Width%,Escapes. WidthPixels=0 использует общий предел; Width%=25–300 — масштаб строки, умножаемый на общий. Сохраняйте <cr>/<lf>/<tab>/<lt>, moekuri-tags-v1 и параметры printf. Старые CSV/TXT поддерживаются; четвёртый столбец TXT — масштаб. JSON сохраняет только текст. Команды и @-идентификаторы не меняются.

VS 2022 C++/Windows SDK: build.bat либо build.ps1 -Package. Внутренняя DLL встроена. Windows Actions публикует теги v* по строгому списку, включая font.ttf. Нужны оригинальные ресурсы игры.

Спасибо [lxgw/kose-font](https://github.com/lxgw/kose-font) за **Xiaolai Font**, [OFL 1.1](FONT-LICENSE-OFL.txt), [уведомления](FONT-NOTICES.txt). DXA: [DxLib for Linux](https://github.com/dragoon2014/dxlib-for-linux). Отдельная лицензия на C++ проекта пока не назначена.

## ar-العربية

أداة ترجمة وتعديل للنسخة اليابانية MoeKuri 2 1.10. يبدأ الواجهة بالإنجليزية ثم تحفظ اللغة المختارة. اسم الملف المقترح يتبع لغة الأداة: moekuri_cn/en/jp/kr/ar.exe ويمكن تغييره. تغيير لغة الواجهة لا يترجم القاموس تلقائياً.

استيراد موحد لـ TXT/CSV/TSV/JSON واستخراج EXE الأصلي وقراءة EXE المترجم وفك DXA. اختر صفاً بالأعلى وعدّل ترجمته بالأسفل؛ النقر المزدوج ينقل إلى المحرر. بحث وتصفية غير المترجم وصفحات من 80 صفاً وانتقال لصفحة محددة.

استبعاد ترجمة DXA مفعّل افتراضياً: استخراج وتصدير وإنشاء نصوص EXE فقط دون المفاتيح المشتركة. عطّله للوصف والحوار والمهارات. المثال 1.10trans_omidxa.txt يحوي 1805 صفوف خاصة بـ EXE؛ المثال الكامل 1.10trans.txt.

الخط الخارجي وتحسينه مفعّلان افتراضياً. اختر ملف TTF/OTF؛ يُقرأ الاسم الحقيقي ويُصدّر الخط بجانب EXE. خيار [font.dxa] يضغط الخط ويسجله ويعيد توجيه طلبات اللعبة. الأولوية: font.dxa عند تفعيله ثم font.otf ثم font.ttf ثم خط النظام. الملف المفقود أو التالف ينتقل للبديل. عطّل الخارجي لتضمين الخط في EXE واحد.

فك وإعادة حزم DXA v4 بأسماء CP932 ومفتاح اللعبة. تُحفظ بايتات الموارد وUTF-16، والقواميس UTF-8. تُرفض الملفات الموجودة والمسارات غير الآمنة والروابط. احفظ الأرشيف خارج مجلد المصدر. لا تستعيد هذه الأداة مصدر C++ من EXE.

أعمدة CSV: Original,Translated,WidthPixels,Width%,Escapes. WidthPixels=0 يرث العرض الافتراضي وWidth%=25–300 مقياس الصف مضروباً بالعام. احتفظ بـ <cr>/<lf>/<tab>/<lt> وmoekuri-tags-v1 ومعاملات printf. تدعم CSV/TXT القديمة؛ العمود الرابع في TXT للمقياس. JSON للنص فقط. الأوامر ومعرّفات @ محفوظة.

ابنِ بواسطة VS 2022 C++ وWindows SDK: build.bat أو build.ps1 -Package. DLL الداخلية مضمّنة. تنشر Actions على Windows وسوم v* بملفات محددة وfont.ttf فقط. تبقى موارد اللعبة الأصلية مطلوبة.

شكراً لـ [lxgw/kose-font](https://github.com/lxgw/kose-font) على **Xiaolai Font** تحت [OFL 1.1](FONT-LICENSE-OFL.txt)، [الإشعارات](FONT-NOTICES.txt). مرجع DXA: [DxLib for Linux](https://github.com/dragoon2014/dxlib-for-linux). لم تحدد رخصة مستقلة لمصدر C++ بعد.

## es-español

Editor de traducción y modding para MoeKuri 2 japonés 1.10. Primer inicio en inglés; después recuerda el idioma. Nombre sugerido según interfaz: moekuri_cn/en/jp/kr/es.exe, editable al guardar. El idioma de interfaz no traduce el catálogo.

Importación unificada TXT/CSV/TSV/JSON, extracción de EXE original, recuperación de EXE traducido y extracción DXA. Selecciona una fila arriba y edita debajo; doble clic activa el editor. Búsqueda, filtro sin traducir, páginas de 80 filas y salto de página.

Excluir DXA está activado: extracción/exportación/generación conservan solo textos EXE sin claves compartidas. Desactívalo para descripciones/diálogos/habilidades. 1.10trans_omidxa.txt contiene 1805 entradas EXE; 1.10trans.txt es completo.

Fuente externa y optimización activadas por defecto. Elige TTF/OTF; detecta el nombre real y exporta la fuente junto al EXE. [font.dxa] comprime, registra y redirige las fuentes del juego. Prioridad: font.dxa habilitado → font.otf → font.ttf → sistema. Si falta o está dañado pasa al siguiente. Desactiva externa para incrustar en un único EXE.

DXA v4 con nombres CP932 y clave del juego: extrae, modifica y vuelve a empaquetar. Conserva bytes y UTF-16 de recursos; catálogos UTF-8. Rechaza archivos existentes, rutas peligrosas y enlaces. Guarda el archivo fuera de la carpeta fuente. No reconstruye código C++ de EXE.

CSV: Original,Translated,WidthPixels,Width%,Escapes. WidthPixels=0 hereda el valor global; Width%=25–300 es escala por fila multiplicada por la global. Conserva <cr>/<lf>/<tab>/<lt>, moekuri-tags-v1 y parámetros printf. Compatibles CSV/TXT anteriores; cuarta columna TXT para escala. JSON solo textos. Comandos e identificadores @ conservados.

Compila con VS 2022 C++/Windows SDK: build.bat o build.ps1 -Package. DLL interna incrustada. Actions Windows publica tags v* con lista estricta y font.ttf. Se necesitan recursos originales.

Gracias a [lxgw/kose-font](https://github.com/lxgw/kose-font) por **Xiaolai Font**, [OFL 1.1](FONT-LICENSE-OFL.txt), [avisos](FONT-NOTICES.txt). Referencia DXA: [DxLib for Linux](https://github.com/dragoon2014/dxlib-for-linux). El C++ del proyecto aún no tiene licencia independiente.

