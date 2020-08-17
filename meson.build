project('gRoulette', 'c')

add_global_link_arguments('-rdynamic', language : 'c')

gtk = dependency('gtk+-3.0')

executable('groulette', sources: [
                        'groulette.c'
                        ],
                        include_directories: '.',
                        dependencies: [gtk],
                        )
