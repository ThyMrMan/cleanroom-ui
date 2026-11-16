"""Install the Flex SDK 4.16.1 with playerglobal 11.2 into a folder, each download checked against its known hash.
CI runs it; an author can too, then points CRUI_FLEX at the folder.

    python -m crui.flex <folder> [--downloads <folder>]   # files already in --downloads are not fetched again

See: docs/swf_framework.md#mxmlc-setup
"""

import argparse
import hashlib
import sys
import urllib.request
import zipfile
from pathlib import Path

#: Apache's SDK archive and its MD5 (Apache's own .md5).
SDK_URL = 'https://archive.apache.org/dist/flex/4.16.1/binaries/apache-flex-sdk-4.16.1-bin.zip'
SDK_MD5 = '8841c64bd5e32f8575eba86e2574873a'
#: Adobe's playerglobal 11.2, from the Internet Archive (Adobe's address now redirects); its SHA-1.
PLAYERGLOBAL_URL = ('https://web.archive.org/web/20160611061401id_/'
                    'http://fpdownload.macromedia.com/get/flashplayer/updaters/11/playerglobal11_2.swc')
PLAYERGLOBAL_SHA1 = 'dc0f3d16f12d5a228349beae700de4c3f6ba0839'


def fetch(url: str, downloads: Path, algorithm: str, expected: str) -> Path:
    """`url` in `downloads` (fetched when missing); ValueError when its hash is not `expected`."""
    path = downloads / url.rsplit('/', 1)[1]
    if not path.is_file():
        downloads.mkdir(parents=True, exist_ok=True)
        request = urllib.request.Request(url, headers={'User-Agent': 'cleanroom-ui-build'})
        with urllib.request.urlopen(request, timeout=300) as response:
            path.write_bytes(response.read())
    found = hashlib.new(algorithm, path.read_bytes()).hexdigest()
    if found != expected:
        path.unlink()
        raise ValueError(f'{url}: {algorithm} {found}, expected {expected}')
    return path


def install(flex: Path, downloads: Path) -> Path:
    """The SDK unpacked into `flex`, playerglobal under libs/player/11.2 and flex-config.xml pointed at it."""
    with zipfile.ZipFile(fetch(SDK_URL, downloads, 'md5', SDK_MD5)) as archive:
        archive.extractall(flex)
    player = flex / 'frameworks' / 'libs' / 'player' / '11.2'
    player.mkdir(parents=True, exist_ok=True)
    swc = fetch(PLAYERGLOBAL_URL, downloads, 'sha1', PLAYERGLOBAL_SHA1)
    (player / 'playerglobal.swc').write_bytes(swc.read_bytes())
    config = flex / 'frameworks' / 'flex-config.xml'
    template = config if config.is_file() else flex / 'frameworks' / 'flex-config-template.xml'
    text = template.read_text(encoding='utf-8')
    config.write_text(text.replace('{playerglobalHome}', 'libs/player'), encoding='utf-8')
    return flex


def main(argv=None) -> int:
    """Install into the folder named; 1 when a download's hash is wrong."""
    parser = argparse.ArgumentParser(prog='python -m crui.flex', description=__doc__.splitlines()[0])
    parser.add_argument('folder', type=Path)
    parser.add_argument('--downloads', type=Path, help='where the downloads are kept (<folder>/downloads)')
    args = parser.parse_args(argv)
    try:
        print(f'[flex] {install(args.folder, args.downloads or args.folder / "downloads")}')
    except ValueError as error:
        print(f'[flex] {error}')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
