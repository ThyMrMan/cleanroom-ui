"""The one download: the framework always installed, the dialogue menu an option, and a reproducible zip.

See: docs/swf_framework.md#fomod
"""

import xml.etree.ElementTree as ET
import zipfile

from crui import fomod, menus, translations

FILES = (*fomod.REQUIRED, 'Interface/CRUI_Pipboy.swf', 'Interface/Translations/CleanroomUI_en.txt',
         'Interface/Translations/CleanroomDialogue_en.txt', 'Scripts/CRUI_Sex.pex')


def _built(root):
    """A mod folder holding FILES; its path."""
    for rel in FILES:
        (root / rel).parent.mkdir(parents=True, exist_ok=True)
        (root / rel).write_text(rel, encoding='utf-8')
    return root


def test_the_dialogue_files_are_the_dialogue_release():
    """DIALOGUE_FILES name the dialogue DLL, the dialogue menu's movie and the dialogue release's words."""
    dialogue = menus.BY_NAME['dialogue'].movie
    assert {fomod.part(rel) for rel in ('F4SE/Plugins/CleanroomDialogue.dll', dialogue,
                                        f'Interface/Translations/{translations.DIALOGUE}_fr.txt')} == {fomod.DIALOGUE}
    assert {fomod.part(rel) for rel in ('F4SE/Plugins/CleanroomUI.dll', 'Interface/CRUI_Hud.swf',
                                        'Interface/Translations/CleanroomUI_fr.txt')} == {fomod.CORE}


def test_the_download_splits_the_mod_folder_into_its_parts(tmp_path):
    """Every file lands once, in its part's folder, beside fomod/ModuleConfig.xml and info.xml."""
    path = fomod.write(_built(tmp_path / 'built'), tmp_path / 'stage', tmp_path / 'dist')
    names = set(zipfile.ZipFile(path).namelist())
    assert names == {f'{fomod.part(rel)}/{rel}' for rel in FILES} | {'fomod/ModuleConfig.xml', 'fomod/info.xml'}
    assert path.name == f'CleanroomUI-{fomod.version()}.zip'


def test_the_same_files_give_the_same_zip(tmp_path):
    """Two builds of one mod folder are byte for byte the same."""
    built = _built(tmp_path / 'built')
    first = fomod.write(built, tmp_path / 'stage', tmp_path / 'one').read_bytes()
    assert fomod.write(built, tmp_path / 'stage', tmp_path / 'two').read_bytes() == first


def test_the_installer_offers_the_dialogue_menu_unless_xdi_is_installed():
    """The core folder is required; the dialogue folder is one plugin, Recommended, and Optional with XDI's DLL."""
    config = ET.fromstring(fomod.module_config())
    assert config.find('requiredInstallFiles/folder').get('source') == fomod.CORE
    [plugin] = config.iter('plugin')
    assert plugin.find('files/folder').get('source') == fomod.DIALOGUE
    kind = plugin.find('typeDescriptor/dependencyType')
    assert kind.find('defaultType').get('name') == 'Recommended'
    pattern = kind.find('patterns/pattern')
    assert pattern.find('dependencies/fileDependency').get('file') == fomod.XDI
    assert pattern.find('type').get('name') == 'Optional'
    assert ET.fromstring(fomod.info()).findtext('Version') == fomod.version()


def test_a_mod_folder_missing_a_required_file_is_refused(tmp_path):
    """missing names each REQUIRED file the folder lacks."""
    built = _built(tmp_path)
    (built / 'F4SE/Plugins/CleanroomDialogue.dll').unlink()
    assert fomod.missing(built) == ['F4SE/Plugins/CleanroomDialogue.dll']
