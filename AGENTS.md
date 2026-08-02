# decade

Stabile Leitplanken für die Arbeit am decade-Code: Architektur und Konventionen. Build und Betrieb stehen in [betrieb.md](betrieb.md), offene Punkte als Issues.

## Zweck

C++23-Desktopanwendung für Kalender und Zeitachsen. Aus einem Prototyp gewachsen; sie trägt noch technische Schulden (Gott-Klassen, unklare Besitzverhältnisse). Ziel ist evolutionäres Refactoring — schrittweise, verhaltenserhaltend, ohne Rewrite (siehe [Refactoring](#refactoring)).

Getragen von wxWidgets (GUI), OpenGL via libepoxy (Rendering), ICU (Kalender/Locale), Boost.Serialization (XML-Projektdateien), FreeType, csv2 und Bullet (Picking). Die vollständige Liste samt Untermodulen steht in `CMakeLists.txt` und `external/` (Stand via `git submodule status`). Build, Tests, kopflose Läufe und die Lint-Gate-Befehle: [betrieb.md](betrieb.md).

## Architektur

### Ziele

- Den Startpfad klein und testbar halten.
- UI-Verdrahtung vom App-Bootstrap isolieren.
- Den Datenfluss zwischen Stores, Panels und Renderer explizit halten.
- Klare Schichtung und selbsterklärende Namen, damit der Code navigierbar bleibt — siehe [Selbsterklärender Code](#selbsterklärender-code).

### Schichten & Schichtregeln

Die Codebasis folgt einer Architektur mit vier Schichten im Sinn der Clean Architecture (https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html). Abhängigkeiten fliessen nur **nach innen** (Presentation → Application → Domain; Infrastructure wird von der Application konsumiert). Die Domain kennt nichts anderes; die Infrastructure kennt nur die Domain-Typen, die sie serialisiert. Neuer Code muss angeben, zu welcher Schicht er gehört, und deren Abhängigkeitsgrenzen einhalten.

- **Presentation** — das Hauptfenster (`MainFrame`), wxWidgets-Panels, GL-Canvas-Wrapper, Menüs, Dialoge, Datei-Befehle. Jedes Panel besitzt seine Widgets und bietet Signale mit derselben Schnittstelle wie sein Store. Das Fenster besitzt nur Widgets: keine Stores, kein Bus, keine Laufzeitoptionen.
- **Application** — Composition Root (`AppComposition`), EventBus, Binder, Projektdokument, Startskript, Rendering-Adapter, App-Lifecycle: konstruiert, besitzt und verdrahtet, enthält aber selbst keine Domänenlogik.
- **Domain** — Value Objects, Stores, Transformationslogik, sigslot-Signale — UI-agnostisch und Boost-frei (kein `friend boost::serialization::access`, keine `serialize`-Member).
- **Infrastructure** — Rendering (OpenGL, FreeType), Persistenz (XML/CSV/PNG), Hit-Testing (Bullet) — nicht-intrusiv, kennt nur Domain-Typen.

Regeln:

1. Der Prozesseinstiegspunkt enthält keine Business- oder UI-Verdrahtungslogik.
2. Der App-Lifecycle konstruiert High-Level-Objekte, kennt aber keine Panel-/Store-Interna.
3. Die Composition Root besitzt alle Lifetimes. Die Verdrahtung lebt getrennt davon im Binder (freie Funktionen) — Komponenten kennen einander nicht direkt. Was erst mit dem GL-Kontext entsteht (Rendering-Adapter, Verdrahtung), liegt in einem `optional` und wird beim Zerstören des Fensters aufgelöst, solange Panels und Canvas noch leben.
4. Die Domain bleibt UI-agnostisch — kein wx, kein GL, kein Boost.
5. Stores publizieren ihren Zustand selbst auf einem eingesetzten Topic; Konsumenten abonnieren über den Bus. Kein Store besitzt ein eigenes Signal, und niemand hängt sich an einen Store statt an den Bus.
6. Infrastructure nimmt Domain-Typen per Referenz; sie darf nicht von Presentation abhängen.
7. Die Kommandozeile wird an genau einer Stelle gelesen und in ein Options-Objekt übersetzt (`src/application/runtime_options.hpp`); Umgebungsvariablen liest die Anwendung nicht.
8. Die Serialisierung ist nicht-intrusiv: Sie arbeitet nur über öffentliche APIs und besitzt das On-Disk-Format; Domain-Typen wissen nichts von Persistenz.
9. Genau eine Brücke verbindet Application und Rendering-Infrastructure: der Rendering-Adapter mit seinem Scene-Composer. Presentation hängt nie von GL-Typen ab — sie sieht den Scene-Graph nur als GL-freies Read-Model (Snapshot).
10. Der anwendungsweite `LocaleDateFormatter` wird einmal in der Composition Root konstruiert und per Referenz weitergereicht — die Locale-Konfiguration bleibt an einer Stelle.
11. Das geöffnete Projekt (alle Stores plus Dateipfad) ist ein Objekt: `ProjectDocument`. Laden, Speichern und CSV-Austausch gehen nur darüber; niemand reicht die sechs Stores einzeln durch.

### Domain-Muster: Value Objects & Stores

- **Value Objects (https://martinfowler.com/bliki/ValueObject.html)** halten Daten plus die Abfragen darauf und kapseln ihren Zustand: Datenmember `private`, Zugriff über konstante Accessors, Änderung nur über benannte Setter — so bleiben Invarianten und On-Disk-Format an einer Stelle. Kein Signal, keine Serialisierung, kein `friend` → Rule of Zero (https://en.cppreference.com/w/cpp/language/rule_of_three), frei kopierbar.
- **Stores** kombinieren ein Value Object mit einem eingesetzten `domain::StateTopic` und einer Re-Entry-Guard. Sie haben Identität → explizit nicht kopierbar. Ein Store besitzt kein eigenes Signal: er bekommt sein Topic per Konstruktor-Referenz (der Port, `domain/state_topic.hpp`) und veröffentlicht direkt darauf. Wer das Topic besitzt, entscheidet die äussere Schicht — in der Anwendung der `EventBus`, im Test ein lokales Signal. Das Topic trägt den Value (`StateTopic<Value>` = `signal<const Value&>`), sodass Konsumenten mit kopierbaren Value Objects arbeiten; der Store bietet `Receive`/`Send`/`Get`, keine eigene Query-Delegation.
- Value Object und Store liegen in **separaten Dateien**, benannt nach der Hauptklasse (`date_group.hpp` + `date_group_store.hpp`); der Value-Object-Header hat keine Store-Abhängigkeit.

### Datums- & Intervallsemantik

- `Date` (proleptischer Gregorianischer Kalender, expliziter Invalid-State) und `DatePeriod` sind die datenbankfreie Schnittstelle. Die Kalenderberechnungen sind an ICU delegiert und bewusst auf genau zwei Stellen beschränkt: das interne Arithmetik-Backend und den `LocaleDateFormatter` (sprach- und locale-abhängiges Parsen und Formatieren für GUI und CSV). Ein Wechsel der Datumslibrary heisst: genau diese zwei Stellen neu implementieren — sonst greift niemand direkt auf ICU-Datum-APIs zu.
- Persistiert wird als ISO-8601-String (https://de.wikipedia.org/wiki/ISO_8601); der frühere Formatbruch weg von Boost.DateTime liegt als Historie in [#48](https://github.com/schneeregenflocke/decade/issues/48).
- `DatePeriod` ist **überall im Modell einheitlich halb-offen `begin, end)`** (zur Begründung: [Dijkstra, EWD831 (https://www.cs.utexas.edu/users/EWD/transcriptions/EWD08xx/EWD831.html)) — `LengthDays()` ist `end - begin`, `Last()` der Tag vor `end`, und ein Zeitraum ohne enthaltenen Tag (`end <= begin`) ist *null* (die Stores verwerfen null Periods). Nutzer denken in *inklusiven* "von .. bis"-Daten; die Umrechnung passiert an genau zwei nutzerseitigen Grenzen — Datumstabellen-Panel und CSV-I/O — via `PeriodFromInclusiveDates()` beim Eingang und `Last()` bei der Anzeige. Nirgends sonst soll ±1-Tag-Arithmetik auftauchen; es soll so bleiben.

### Ereignisfluss (EventBus via sigslot)

Die komponentenübergreifende Kommunikation läuft über einen in-process **EventBus** mit je einem typisierten Topic pro Domain-Ereignis. Stores publizieren direkt darauf (siehe [Domain-Muster](#domain-muster-value-objects--stores)); die Verdrahtung der Konsumenten ist zentral in `app_binder::Bind`. Einen neuen Zustand fügst du als Boost-freies Value Object plus `*Store` in `domain/`, ein Panel in `presentation/`, ein Topic im `EventBus` und die Konsumenten im Binder hinzu; persistierter Zustand ergänzt `save`/`load` in `infrastructure/persistence/value_serialization.hpp` und eine Zeile in `project_io`.

**Zwei Richtungen, zwei Regeln.** Ein Panel-Edit ist ein **Befehl** und geht direkt an den besitzenden Store — dessen `Receive*` ist der einzige Ort, an dem kanonischer Zustand entsteht. Der neue Zustand ist eine **Tatsache** und geht über den Bus. Legten Panels ihre Edits auf dasselbe Topic, das sie abonnieren, gäbe es Rückkopplungen. Nur wo ein Produzent in der Presentation sitzt und deshalb kein Topic eingesetzt bekommt (Schriftwahl, Baumauswahl), hängt `detail::Forward` das Panelsignal ans Topic. Der Szenenbaum ist die Gegenprobe: Er spiegelt den Rendergraphen und zeigt nur an — jede Zeile seines Detailgrids ist read-only, weil ein Editierfeld dort ein zweiter Schreibpfad auf Zustand wäre, den der Rebuild ohnehin neu erzeugt.

Die Verdrahtung selbst ist eine Lebensdauer, kein Aufrufpaar: `AppWiring` verbindet beim Bauen und trennt beim Zerstören. In der Composition Root steht sie als letztes Mitglied und stirbt damit vor Stores und Bus.

**Bearbeiten im Canvas** folgt derselben Regel, nur zeitversetzt: Der Editor (`TitleTextEditor`) hält einen Puffer und veröffentlicht bei jedem Anschlag ein GL-freies Read-Model (`TextEditView`), aus dem der Renderer Text, Cursor und Auswahl zeichnet. Kanonisch wird der Text erst mit Enter — dann geht er als *ein* Befehl an den Store; Esc verwirft den Puffer. So ist eine Bearbeitung genau ein Zustandswechsel, nicht einer je Taste. Die Tastencodes bleiben in der Presentation: das Canvas übersetzt sie in ein `TextInputEvent`, der Editor kennt nur dessen Bedeutung.

## Entscheide

### Sprachstand und Header-Guards

- C++23, keine Compiler-Erweiterungen.
- Header-Guards benutzen den Dateinamenstil: der grossgeschriebene Dateiname mit dem Punkt vor dem Suffix als `_`, z. B. `main_window.hpp` → `MAIN_WINDOW_HPP`, `gl_canvas.hpp` → `GL_CANVAS_HPP`. Kein Verzeichnispfadpräfix. Das konsequent in `#ifndef`, `#define` und dem abschliessenden `#endif  // <GUARD>`-Kommentar anwenden. (Die clang-tidy-Prüfung `llvm-header-guard`, die sonst einen Full-Path-Stil erzwingen würde, ist in `.clang-tidy` deaktiviert — so lassen.)

### Selbsterklärender Code

Der Code kommuniziert seine Absicht selbst; Prosa ist die Ausnahme. Leitplanke ist P.1 «Express ideas directly in code» der C++ Core Guidelines (https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rp-direct). Was der Code oder ein Befehl schon zeigt, wird nicht zusätzlich dokumentiert.

- **Namen tragen den Zweck, nicht den Mechanismus** — auf allen Ebenen: Variablen, Funktionen, Klassen, Member. Anker: Intention-Revealing Selector (Kent Beck, «Smalltalk Best Practice Patterns») für Namen; Intention-Revealing Interfaces (Eric Evans, DDD Reference, https://www.domainlanguage.com/ddd/reference/) für Schnittstellen; General Naming Rules (https://google.github.io/styleguide/cppguide.html#General_Naming_Rules): für Lesbarkeit optimieren, keine kryptischen Abkürzungen.
- **Struktur erklärt sich selbst:** kleine Einheiten mit einer Verantwortung; eine Abstraktionsebene pro Funktion (Single Level of Abstraction Principle, SLAP); tiefe Module — kleine Schnittstelle, viel Funktionalität dahinter (Deep Modules; John Ousterhout, «A Philosophy of Software Design», https://web.stanford.edu/~ouster/cgi-bin/book.php).
- **Kommentare sind sparsam** und erklären nur das nicht-offensichtliche Warum (Entscheidung, Trade-off), nie das Was (NL.1 der Core Guidelines, https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-naming). Ein Kommentar, der beschreibt, *was* der Code tut, ist ein Hinweis, den Code klarer zu machen — nicht, den Kommentar zu behalten.

Konkret, nach Google C++ Style (https://google.github.io/styleguide/cppguide.html#Naming) (in Kraft):

- Typen: `PascalCase` (`DateGroup`).
- Funktionen & Methoden: `PascalCase` (`GetDateGroups()`); triviale Accessor/Mutator dürfen `snake_case` wie ihr Member heissen (`set_count()`).
- Klassen-Datenmember: `snake_case` **mit Trailing-Underscore** (`date_format_`). Struct-Member ohne Underscore. Diese Member-Regel wird vom clang-tidy-Gate erzwungen (`readability-identifier-naming` (https://clang.llvm.org/extra/clang-tidy/checks/readability/identifier-naming.html) in `.clang-tidy`); ein Member ohne Underscore bricht den Build.
- Locals: `snake_case`. Konstanten/Enumeratoren: `kPascalCase` (`kColorScale`).
- Der Store-Suffix ist einheitlich `…Store` (kein `…Storage`) — bei Typen **und** bei Member-/Parameternamen (`…_store`, nicht `…_storage`).
- Renames zur Vereinheitlichung von Schreibweise und Bezeichnern sind erwünscht. Wenn umbenannt wird, dann **vollständig und konsistent** über alle Vorkommen (Deklaration, Definition, Aufrufstellen, Tests, Doku) — kein halber Rename, der zwei Schreibweisen nebeneinander stehen lässt. Den Build danach grün halten (Compile + `ctest` + clang-tidy-Gate).

### Refactoring

Ziel ist selbsterklärender Code; Refactoring bringt ihn schrittweise dorthin.

- Sicherheit vor Struktur. Vor einer strukturellen Änderung an einer God Class oder einem anderen Code Smell (https://en.wikipedia.org/wiki/Code_smell) zuerst eine Black-Box-Schutzschicht ergänzen: einen Charakterisierungstest (https://en.wikipedia.org/wiki/Characterization_test) mit Eingabe und dem aktuellen Output als eingefrorener Erwartung. Kein Umbau ohne diese Absicherung.
- Verhalten beim Aufräumen nicht ändern. Formatting, Umbenennungen, Warnungsbereinigung und Verhaltensänderungen sind getrennte Schritte oder Commits. Beim Reparieren einer Warnung nie stillschweigend die Semantik ändern.
- Kleine, umkehrbare Schritte. Ein Commit, ein Ziel. Diffs klein genug halten, um sie sauber zurückdrehen zu können. Für grössere Umbauten: Mikado-Methode (Ellnestam/Brolund) — Ziel notieren, Voraussetzungen explorieren, bei Bruch zurückrollen statt durchdrücken.
- Refactoring ist evolutionär, kein Rewrite. Erst das Risiko senken, dann entlang der tragenden Abstraktion schneiden.
- Reine Formatierungscommits isolieren und in `.git-blame-ignore-revs` eintragen (`git blame --ignore-revs-file` (https://git-scm.com/docs/git-blame)), damit die Historie lesbar bleibt.
- Schrittweiser Ablauf: zuerst stabilisieren (Charakterisierungstests, Smoke-Pfade, Baseline-Output) → dann aufteilen (kleine Nähte extrahieren — Seams nach Michael Feathers, «Working Effectively with Legacy Code» —, Verhalten unverändert) → danach umbenennen (Absicht sichtbar machen, ohne Semantik zu ändern) → zuletzt entkoppeln (Kopplung erst entfernen, wenn die Form bereits sicher ist).
- Wenn etwas nicht sofort geändert werden kann, wird es ein Issue (siehe [Offene Punkte](#offene-punkte)), damit es nicht verloren geht.
- Beim Arbeiten mit einer Datei aufgefallene Verstösse gegen die Konventionen dieser Datei werden — auch wenn sie nicht Teil der Aufgabe sind — als Issue angelegt.

### Stil

Sprache und Doku-Regeln stehen im Superproject (`~/homelab-superproject/AGENTS.md`). Hier gilt zusätzlich: Kommentare auf Deutsch, Code-Identifier englisch.

### Ownership & Lifetimes

- Niemals rohes `new`/`delete` verwenden. Ownership immer über Smart Pointer ausdrücken, damit die Lifetime im Typsystem kodiert ist, Ausnahmen keine Ressourcen leaken und Ownership-Übergaben an Call-Sites explizit sind (vgl. C++ Core Guidelines, Resource management (https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-resource)). wx-Widgets werden typischerweise via `.release()` an wx-owned Parents übergeben (wx besitzt dann die Lifetime).
- Reihenfolge der Wahl: Wertsemantik / Stack / Container zuerst → `std::unique_ptr` für exklusiven Besitz → `std::shared_ptr` *nur*, wenn Besitz nachweislich geteilt ist (ein Design-Signal, kein Default) → `std::weak_ptr` gegen Zyklen.
- Rohzeiger sind nie Besitzer (https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-ptr); als nicht-besitzende Beobachter sind sie nur in Parametern und Locals zulässig. **Raw-Pointer-Datenmember sind verboten** — auch nicht-ownende: ein rohes `T*`-Member kodiert weder Ownership noch Lifetime und ist die klassische Dangling-Pointer-Falle. Drücke die Beziehung stattdessen im Typsystem aus:
  - **Nicht-ownende Referenz auf ein von `wxTrackable` abgeleitetes Objekt** (wx-owned Parents/Children — `wxWindow`, `wxEvtHandler` und die meisten wx-Klassen): `wxWeakRef<T>` (https://docs.wxwidgets.org/3.2/classwx_weak_ref_3_01_t_01_4.html) verwenden. Es setzt sich automatisch auf null zurück, wenn das referenzierte Objekt zerstört wird (siehe wxTrackable (https://docs.wxwidgets.org/3.2/classwx_trackable.html)).
  - **Nicht-ownende Referenz auf ein wx-owned Objekt, das *nicht* `wxTrackable` ist** (z. B. `wxPGProperty` erbt von `wxObject`, daher kompiliert `wxWeakRef` nicht): **gar keinen Pointer cachen**. Das Objekt bei Bedarf vom Owner holen — aus dem Event, das es trägt (`wxPropertyGridEvent::GetProperty()`), oder per Namenssuche am Owner (`wxPropertyGrid::GetPropertyByName()`) — mit einer stabilen Namenskonstante, die Erzeugung und Lookup gemeinsam verwenden. Der temporäre `T*`, den eine wx-API am Call-Site zurückgibt, ist in Ordnung; das Speichern als Member nicht.
  - **Ein Heap-Objekt besitzen**: ein Smart Pointer (`std::unique_ptr` / `std::shared_ptr`) oder `MakeOwned<T>`, wenn ein wx-Widget an seinen wx-owned Parent übergeben wird.
- Rule of Five (https://en.cppreference.com/w/cpp/language/rule_of_three): Klassen mit expliziten Destruktoren sollten Copy/Move ebenfalls löschen oder defaulten (siehe `MainWindow`, `DateEntryStore`).

### C++-Disziplin

- const-correctness (https://isocpp.org/wiki/faq/const-correctness) konsequent: SSOT für Veränderlichkeit — was nicht verändert wird, ist `const`.
- Include-Disziplin: Include-Dickicht auflösen (Vorwärtsdeklarationen, minimale Includes, include-what-you-use (https://include-what-you-use.org/)). Grösserer struktureller Hebel als Reformatierung, senkt nebenbei Compile-Zeiten.

### Als Header-only konzipiert

Die Codebasis ist **absichtlich als Header-only konzipiert** (`main.cpp` ist die einzige Translation Unit). Beim Hinzufügen von Code lieber bestehende Header direkt erweitern als in `.cpp` aufzuteilen. Diese Konvention auch bei Refactorings beibehalten. Definitionen, die in einem Header leben, müssen `inline` (https://en.cppreference.com/w/cpp/language/inline) sein (freie Funktionen und out-of-class Member-Definitionen), damit die Single-TU-Regel ODR-Verstösse nicht stillschweigend verdeckt, falls ein Header irgendwann von woanders eingebunden wird (z. B. Tests).

### Warnings, clang-tidy- und Sanitizer-Gate

- **Warnings brechen den Build — behebe sie, unterdrücke sie nicht. Diese Regel gilt sowohl für Compiler-Warnungen als auch für Diagnosen von clang-tidy (https://clang.llvm.org/extra/clang-tidy/).** Kein Finding mit `NOLINT` / `NOLINTNEXTLINE` / `NOLINTBEGIN` (https://clang.llvm.org/extra/clang-tidy/index.html#suppressing-undesired-diagnostics), einem `#pragma`, einem `-Wno-…`-Flag oder einer `.clang-tidy`-Einzelzeilen-Ausnahme einfach ruhigstellen. Ändere den Code so, dass das Finding nicht mehr greift. Umstrukturierung, RAII und korrekte Typannotationen sind Fixes; Unterdrückung ist keiner.
  - **Bevorzuge einen echten Fix, auch wenn die Warnung zunächst "nicht behebbar" wirkt.** Meist ist sie es doch. Beispiel: `cppcoreguidelines-owning-memory` (https://clang.llvm.org/extra/clang-tidy/checks/cppcoreguidelines/owning-memory.html) über einer rohen C-Ressource ist erfüllt, wenn Ownership mit `gsl::owner<>` aus der GSL (https://github.com/microsoft/GSL) — das Projekt verlinkt bereits `Microsoft.GSL::GSL` — und/oder einem `std::unique_ptr` mit Custom-Deleter ausgedrückt wird — siehe `src/infrastructure/graphics/png_writer.hpp`. Eine Regel, die für den Projektstil wirklich nicht anwendbar ist, wird **einmal, global** in `.clang-tidy` mit Kommentar deaktiviert (wie bereits `-modernize-use-trailing-return-type`, `-llvm-header-guard`, …) — niemals verteilt pro Zeile.
  - **Unterdrückung ist nur als letzter Ausweg erlaubt, und zwar nur für Konstrukte, die uns ein Third-Party-C-API vertraglich aufzwingt und die im Code nicht anders lösbar sind.** Das kanonische (und derzeit einzige zugelassene) Beispiel ist das zwingende `setjmp`/`longjmp`-Error-Handling von libpng in `src/infrastructure/graphics/png_writer.hpp`. Wenn du unterdrücken musst: scoping des `NOLINT` auf die **konkreten Check-Namen** (niemals ein nacktes `NOLINT`), auf die engste Zeile beschränken und einen Kommentar hinzufügen, der erklärt, *warum* das nicht behebbar ist. Wenn eine ganze Abhängigkeit eine Warnungsklasse unvermeidbar macht, ersetze lieber diese Abhängigkeit, statt Unterdrückungen zu verteilen.

Neben clang-tidy steht das Sanitizer-Gate `sanitize-address` (address, leak, undefined): es baut den Baum instrumentiert neu und lässt die Testsuite darunter laufen. Auch dieser Baum wird bei **null Befunden** gehalten; ein Sanitizer-Treffer wird behoben, nicht unterdrückt. Wer Verhalten ändert, das im Test nicht abgedeckt ist, deckt es zuerst ab — der Sanitizer sieht nur, was auch läuft.

Die Gate-Befehle (Durchsetzungs-Targets, voller Lauf, Auto-Fix, clang-format, CI) stehen in [betrieb.md](betrieb.md), Abschnitt Build-Prüfungen.

### Prinzipien

Verbindliche Entwurfsprinzipien. Die etablierten Fachbegriffe sind hier — wie im ganzen Dokument — bewusst als semantische Anker gesetzt (Semantic Anchors, https://github.com/LLM-Coding/Semantic-Anchors): Der Begriff aktiviert das dahinterliegende Wissen, bei Menschen wie bei Coding-Agents, präziser als jede Umschreibung. Als allgemeine C++-Leitlinie gelten durchgehend die C++ Core Guidelines (https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#main), aus denen viele der Anker unten stammen.

- Am offiziellen Manual prüfen, nicht aus dem Gedächtnis: bei Arbeit mit wxWidgets, OpenGL, ICU, Boost, clang-tidy oder CMake die Doku der **hier genutzten Version** lesen — Verhalten, Flags und Defaults ändern sich zwischen Versionen.
- Single Responsibility Principle (https://en.wikipedia.org/wiki/Single-responsibility_principle) und Separation of Concerns (https://en.wikipedia.org/wiki/Separation_of_concerns); geringe Kopplung (https://en.wikipedia.org/wiki/Coupling_%28computer_programming%29), hohe Kohäsion (https://en.wikipedia.org/wiki/Cohesion_%28computer_science%29).
- Domain-Driven Design (https://www.domainlanguage.com/ddd/reference/) — siehe das [Domain-Muster](#domain-muster-value-objects--stores) — und Clean Architecture (https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html) — siehe [Schichten & Schichtregeln](#schichten--schichtregeln).
- DRY (https://en.wikipedia.org/wiki/Don%27t_repeat_yourself) als Wissens-DRY, massgeblich ist Single Source of Truth (https://en.wikipedia.org/wiki/Single_source_of_truth): jedes Stück Wissen (Regel, Konstante, Domänenentscheidung) hat genau eine autoritative Repräsentation — nicht jede ähnlich aussehende Zeile zusammenfalten. Aber: Duplikation ist billiger als die falsche Abstraktion (https://sandimetz.com/blog/2016/1/20/the-wrong-abstraction); zwei zufällig identische Blöcke, die *verschiedene* Konzepte ausdrücken, bleiben getrennt — im Zweifel nicht verfrüht abstrahieren (YAGNI, https://en.wikipedia.org/wiki/You_aren%27t_gonna_need_it; KISS, https://en.wikipedia.org/wiki/KISS_principle).
  - Mechanik: wenn dieselbe mehrzeilige Form über mehrere Methoden (oder Panels) wiederkehrt, ziehe sie in einen kleinen Helfer hoch — ein `private`-Member, eine freie Funktion oder eine gemeinsame Basisklasse — statt sie zu kopieren. Etablierte Beispiele: `scene_shapes::FillRectangles` / `AddCenteredText` (Scene-Node-Erzeugung), `runtime_options_detail::FoundString` (Option lesen → `std::optional<std::string>`), `MakeOwned<T>` (parent-owned Widgets), `TablePanelBase` (Tabellen-plus-Add/Delete-Gerüst), `serialization_detail::ColorToArray` / `ColorFromArray` (glm::vec4-Marshalling). Bevorzuge dies gegenüber Makros, weil Makros Lesbarkeit und Debuggability verschlechtern — die expliziten, feldweisen `save`/`load`-Paare in `infrastructure/persistence/value_serialization.hpp` bleiben absichtlich ausgeschrieben, weil sie das On-Disk-Format dokumentieren.
- Principle of Least Astonishment (https://en.wikipedia.org/wiki/Principle_of_least_astonishment): Namen, Signaturen und Verhalten passen zusammen.
- Die kleinste nützliche Abstraktion wählen; expliziten Datenfluss vor versteckter Kopplung bevorzugen (Law of Demeter, https://en.wikipedia.org/wiki/Law_of_Demeter); unübersichtliche Konstrukte einkapseln statt sie zu verbreiten.
- GRASP (https://en.wikipedia.org/wiki/GRASP_%28object-oriented_design%29)-Heuristiken zur Verantwortungszuweisung, wenn du entscheidest, wo Code hingehört: Information Expert, Creator, Controller, Low Coupling / High Cohesion, Indirection, Pure Fabrication, Polymorphism / Protected Variations.
- Stabile Regeln von instabiler Arbeit trennen (diese Datei vs. die Issues, siehe [Offene Punkte](#offene-punkte)).
