# _custom_build.py

import setuptools


settrie_ext = setuptools.Extension('settrie', sources = ['mercury/settrie'], extra_compile_args = ['-std=c17', '-lm', '-Wl', '-c', '-fPIC'])


class build_py(setuptools.command.build_py):

    def run(self):
        self.run_command('build_ext')

        return super().run()


    def initialize_options(self):
        super().initialize_options()

        if self.distribution.ext_modules == None:
            self.distribution.ext_modules = []

        self.distribution.ext_modules.append(settrie_ext)
