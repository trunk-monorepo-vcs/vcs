project('vcs-client', 'c', version: '0.1', license: 'MIT')

api_level = 30
fuse = dependency('fuse3', required: true)
if not fuse.found()
	fuse = dependency('fuse')
	api_level = 26
endif

proxyfs = executable('vcs-client',
                        'src/client.c',
                        'src/proxyfs.c',
                        'src/diff.c',
                        dependencies: fuse,
                        c_args: ['-D_GNU_SOURCE', '-DFUSE_USE_VERSION=@0@'.format(api_level)],
                        install: true,
                        install_dir: get_option('sbindir'))
