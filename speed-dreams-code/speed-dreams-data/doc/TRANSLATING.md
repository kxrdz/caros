# Translating Speed Dreams

Apart from some exceptions that are hard-coded into the engine, most strings
used by Speed Dreams are stored into various XML files scattered across this
repository. UTF-8 text is supported by the game engine.

> **Important note:** as of the time of this writing, the typefaces used by
> this project do not support non-Latin scripts, such as, but not limited to,
> Arabic, Chinese or Japanese. Efforts are planned to solve this, with no
> defined timeline yet.

## Adding a new supported language

[`languagemenu.xml`](../data/data/menu/languagemenu.xml) defines both the UI
layout for the language selection menu, as well as a list of the supported
languages. The former is irrelevant to the scope of this documentation,
whereas the latter is defined as follows:

```xml
<section name="Languages">
    <section name="1">
        <attstr name="code" val="en"/>
        <attstr name="name" val="English"/>
    </section>
    <section name="2">
        <attstr name="code" val="cat"/>
        <attstr name="name" val="Català"/>
    </section>
</section>
```

As shown above, every language is defined as an item inside the `Languages`
section. Since XML does not support arrays, items are defined in incremental
order (i.e., `name="1"`, `name="2"`, etc.), although this is not strictly
required by the engine.

Every language must implement the following two key-value pairs:

- `code`: the language code that will be used later by the engine to search
for translatable strings.
- `name`: the language name, preferably in its native form to make it easier
for native speakers to find it (e.g.: "Català" instead of "Catalan",
or "Deutsch" instead of "German").

Therefore, if support for a new language is planned, a new entry must be added
to the list. For example, if adding support for German, the `Languages`
section must be expanded as follows:

```xml
<section name="Languages">
    <section name="1">
        <attstr name="code" val="en"/>
        <attstr name="name" val="English"/>
    </section>
    <section name="2">
        <attstr name="code" val="cat"/>
        <attstr name="name" val="Català"/>
    </section>
    <section name="3">
        <attstr name="code" val="de"/>
        <attstr name="name" val="Deustch"/>
    </section>
</section>
```

The language selected by the user shall be written to the user configuration
directory. [`language.xml`](../data/config/language.xml) contains the default
configuration.

## Translating the strings

When rendering a string, the game engine looks it up from a context-specific
XML file. While usually defined by `text` tags, strings can be also defined
by `tip`, `name` or `label` tags. Exceptionally, some other files such as
[`stopracemenu.xml`](../data/data/menu/stopracemenu.xml) define their own,
non-standard tags.

The engine follows the logic below to find a translatable string:

1. Look for the user-configured language in `language.xml`, if any.
2. If defined, append `-` plus the language code (e.g.: `-cat`, `-de`) to
the relevant tag. Examples: `text-cat`, `label-de`.
3. If no language is defined, or the language-specific tag is not defined,
the default tag will be selected (e.g.: `text`, `label`).
4. Read the contents from the selected tag.

Therefore, the game engine shall attempt to read the translated string
whenever possible, falling back to the English version if not available.

In order to translate a given string, a new child attribute must be added to
its section. For example, if working on a German localization for the following
string (`Delete selected player`):

```xml
<section name="delete">
    <attstr name="type" val="image button"/>
    <attstr name="tip" val="Delete selected player"/>
    <attstr name="tip-cat" val="Esborreu el jugador seleccionat"/>
    <attnum name="x" val="550"/>
    <attnum name="y" val="200"/>
    <attnum name="width" val="28"/>
    <attnum name="height" val="28"/>
    <attstr name="disabled image" val="data/img/icon-delete-disabled.png"/>
    <attstr name="enabled image" val="data/img/icon-delete.png"/>
    <attstr name="focused image" val="data/img/icon-delete-focused.png"/>
    <attstr name="pushed image" val="data/img/icon-delete-pushed.png"/>
</section>
```

A new child attribute must be added to the section:

```xml
<section name="delete">
    <attstr name="type" val="image button"/>
    <attstr name="tip" val="Delete selected player"/>
    <attstr name="tip-cat" val="Esborreu el jugador seleccionat"/>
    <attstr name="tip-de" val="Löschen Sie den ausgewählten Spieler"/>
    <attnum name="x" val="550"/>
    <attnum name="y" val="200"/>
    <attnum name="width" val="28"/>
    <attnum name="height" val="28"/>
    <attstr name="disabled image" val="data/img/icon-delete-disabled.png"/>
    <attstr name="enabled image" val="data/img/icon-delete.png"/>
    <attstr name="focused image" val="data/img/icon-delete-focused.png"/>
    <attstr name="pushed image" val="data/img/icon-delete-pushed.png"/>
</section>
```

> **Note:** some sections might simultaneously define `tip`, `text`, `name`
> and/or `label`.

Unfortunately, there is no centralised database of translatable strings,
which makes it trickier to to locate them. However, previous translations
can be used for reference. For example, searching for `-cat"` in XML files
inside this repository should return a comprehensive list of all strings that
were translated to Catalan.
