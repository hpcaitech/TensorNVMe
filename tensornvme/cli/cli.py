import click
from .check import check


@click.group()
def cli():
    pass


cli.add_command(check)

if __name__ == '__main__':
    cli()
