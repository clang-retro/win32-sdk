# MkDocs hook shared by the clang-retro documentation sites.
#
# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) clang-retro contributors.
#
# It lets a page take its text from the repository's own Markdown files, so
# the READMEs stay the one place where things are written down:
#
#   <!-- include README.md -->            the whole file, without its title
#   <!-- include README.md "" -->         the text before the first section
#   <!-- include README.md "Using it" --> one section (and its subsections),
#                                         without its heading; subheadings
#                                         move up to start at level 2
#   <!-- include README.md "Using it" only -->
#                                         the same without its subsections
#
# Links in every page are then fixed up: a link to a Markdown file that is
# part of the site goes to its page, and a link to any other file in the
# repository goes to that file on GitHub, so nothing is left pointing at a
# path that the site does not have.
#
# Settings, in mkdocs.yml:
#
#   extra:
#     retro:
#       root: .                 the repository's root, from mkdocs.yml
#       github: clang-retro/dos-sdk
#       branch: main
#       pages:                  repository files shown as site pages
#         README.md: index.md
#       doxygen: .docs/Doxyfile   an API reference built into <site>/api/
#                                 after the site (from the root; needs
#                                 doxygen, else it is left out with a note)

import logging
import os
import posixpath
import re
import shutil
import subprocess

from mkdocs.exceptions import PluginError

_INCLUDE = re.compile(r'^[ \t]*<!--\s*include\s+(\S+)(?:\s+"([^"]*)"(\s+only)?)?\s*-->[ \t]*$', re.M)
_FENCE = re.compile(r'^\s*(```+|~~~+)')
_HEADING = re.compile(r'^(#{1,6})\s+(.*?)\s*#*\s*$')
_LINK = re.compile(r'(!?)\[((?:[^\[\]]|\[[^\]]*\])*)\]\(\s*<?([^)\s>]+)>?(\s+"[^"]*")?\s*\)')

_settings = {}
log = logging.getLogger("mkdocs.hooks.retro")


def on_config(config):
    retro = dict(config.extra.get("retro", {}))
    base = os.path.dirname(config.config_file_path)
    retro["root"] = os.path.normpath(os.path.join(base, retro.get("root", ".")))
    docs = os.path.relpath(config.docs_dir, retro["root"]).replace(os.sep, "/")
    retro["docs"] = "" if docs == "." else docs
    retro.setdefault("branch", "main")
    retro.setdefault("pages", {})
    if "github" not in retro:
        raise PluginError("extra.retro.github is not set in mkdocs.yml")
    _settings.clear()
    _settings.update(retro)
    return config


def _lines_outside_fences(lines):
    """Yields (index, line, in_fence) for each line."""
    fence = None
    for i, line in enumerate(lines):
        m = _FENCE.match(line)
        if fence is None and m:
            fence = m.group(1)[0] * 3
            yield i, line, True
            continue
        if fence is not None:
            if line.strip().startswith(fence):
                fence = None
            yield i, line, True
            continue
        yield i, line, False


def _section(text, name, path, only=False):
    lines = text.split("\n")
    heads = [(i, len(m.group(1)), m.group(2))
             for i, line, code in _lines_outside_fences(lines)
             if not code and (m := _HEADING.match(line))]
    if name is None:  # the whole file, without its title
        if heads and heads[0][1] == 1:
            del lines[heads[0][0]]
        return "\n".join(lines)
    if name == "":  # before the first section
        start = heads[0][0] + 1 if heads and heads[0][1] == 1 else 0
        end = next((i for i, lvl, _ in heads if lvl > 1), len(lines))
        return "\n".join(lines[start:end])
    for n, (i, lvl, title) in enumerate(heads):
        if title == name:
            end = next((j for j, l2, _ in heads[n + 1:] if only or l2 <= lvl), len(lines))
            body = lines[i + 1:end]
            shift = lvl - 1
            out = []
            for _, line, code in _lines_outside_fences(body):
                m = None if code else _HEADING.match(line)
                if m:
                    line = "#" * (len(m.group(1)) - shift) + " " + m.group(2)
                out.append(line)
            return "\n".join(out)
    raise PluginError(f'{path}: no section "{name}"')


def _github(path, kind):
    repo, branch = _settings["github"], _settings["branch"]
    if kind == "raw":
        return f"https://raw.githubusercontent.com/{repo}/{branch}/{path}"
    return f"https://github.com/{repo}/{kind}/{branch}/{path}".rstrip("/")


def _fix_links(text, source, page):
    """Rewrites the relative links of text, written in the repository file
    `source`, for the site page `page` (both relative to the root)."""
    root, docs, pages = _settings["root"], _settings["docs"], _settings["pages"]

    def fix(m):
        bang, label, target, title = m.groups()
        if target.startswith("#") and source != page:
            # A link to a section of an included file: its page, if the
            # section is shown on a page of its own (pages: README.md#x).
            site = pages.get(source + target)
            if site is not None:
                here = posixpath.dirname(page[len(docs) + 1:] if docs else page)
                return f"{bang}[{label}]({posixpath.relpath(site, here or '.')}{title or ''})"
        if re.match(r"^[a-z][a-z0-9+.-]*:", target, re.I) or target.startswith(("#", "/")):
            return m.group(0)
        path, _, anchor = target.partition("#")
        full = posixpath.normpath(posixpath.join(posixpath.dirname(source), path))
        if full.startswith(".."):
            return m.group(0)
        new = None
        site = pages.get(full)
        if site is None and full.startswith(docs + "/" if docs else ""):
            inside = full[len(docs) + 1:] if docs else full
            if os.path.isfile(os.path.join(root, full)):
                site = inside
        if site is not None:
            here = posixpath.dirname(page[len(docs) + 1:] if docs else page)
            new = posixpath.relpath(site, here or ".")
        elif bang:
            new = _github(full, "raw")
        else:
            kind = "tree" if os.path.isdir(os.path.join(root, full)) else "blob"
            new = _github(full, kind)
        if anchor:
            new += "#" + anchor
        return f"{bang}[{label}]({new}{title or ''})"

    lines = text.split("\n")
    for i, line, code in _lines_outside_fences(lines):
        if not code:
            lines[i] = _LINK.sub(fix, line)
    return "\n".join(lines)


def on_page_markdown(markdown, page, config, files):
    root, docs = _settings["root"], _settings["docs"]
    page_path = posixpath.join(docs, page.file.src_uri) if docs else page.file.src_uri

    def include(m):
        rel, name, only = m.group(1), m.group(2), bool(m.group(3))
        path = os.path.join(root, rel)
        try:
            with open(path, encoding="utf-8") as f:
                text = f.read()
        except OSError as e:
            raise PluginError(f"{page.file.src_uri}: cannot include {rel}: {e}")
        return _fix_links(_section(text, name, rel, only), rel, page_path)

    # Links in the page itself first, so that included text is not fixed twice.
    parts, last = [], 0
    for m in _INCLUDE.finditer(markdown):
        parts.append(_fix_links(markdown[last:m.start()], page_path, page_path))
        parts.append(include(m))
        last = m.end()
    parts.append(_fix_links(markdown[last:], page_path, page_path))
    return "".join(parts)


def on_post_build(config):
    doxyfile = _settings.get("doxygen")
    if not doxyfile:
        return
    if shutil.which("doxygen") is None:
        log.info("doxygen is not installed: the API reference (api/) is left out")
        return
    env = dict(os.environ,
               RETRO_API_OUT=os.path.abspath(config.site_dir),
               RETRO_ROOT=_settings["root"],
               RETRO_DOCS=os.path.abspath(config.docs_dir))
    result = subprocess.run(["doxygen", doxyfile], cwd=_settings["root"], env=env,
                            capture_output=True, text=True)
    if result.returncode != 0:
        raise PluginError(f"doxygen {doxyfile} failed:\n{result.stderr}")
    log.info("API reference written to %s", os.path.join(config.site_dir, "api"))
