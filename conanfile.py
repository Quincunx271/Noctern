from conans import ConanFile
from conans.tools import os_info

class NocternConan(ConanFile):
    requires = (
        'boost/1.74.0',
        'fmt/7.1.2',
    )
    build_requires = (
        'catch2/2.13.3',
    )

    def build_requirements(self):
        if os_info.is_windows:
            self.build_requires('winflexbison/2.5.22')
        else:
            self.build_requires('flex/2.6.4')
