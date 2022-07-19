import click
from tensornvme._C import probe_backend, get_backends


def check_backend(backend: str):
    if backend not in get_backends():
        click.echo(f'Invalid backend: {backend}')
        return
    status = 'x'
    if probe_backend(backend):
        status = u'\u2713'
    click.echo(f'{backend}: {status}')


@click.command(help='Check if backends are available')
@click.option('--backend', type=click.Choice(['all', *get_backends()]), default='all')
def check(backend: str):
    click.echo('Check backends:')
    if backend == 'all':
        for backend in get_backends():
            check_backend(backend)
    else:
        check_backend(backend)
