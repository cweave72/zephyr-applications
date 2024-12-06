import sys
import atexit
import logging
import click
#from rich import inspect

# ProtoRpc modules
from protorpc.cli import get_params
from protorpc.cli.common_opts import cli_common_opts, cli_init
from protorpc.cli.common_opts import CONTEXT_SETTINGS

# Import callset classes
from proto.TestRpc.lib.test import TestCallset

logger = logging.getLogger(__name__)


connections = []

callsets = [(TestCallset, 1)]


def on_exit():
    """Cleanup actions on program exit.
    """
    logger.info("Closing connections on exit.")
    for con in connections:
        con.close()


@click.group(context_settings=CONTEXT_SETTINGS, invoke_without_command=True)
@cli_common_opts
@click.pass_context
def cli(ctx, **kwargs):
    """CLI application for calling Test RPCs.
    """
    global connections

    params = get_params(**kwargs)

    try:
        api, conn = cli_init(ctx, params, callsets)
    except Exception as e:
        logger.error(f"Exiting due to error: {str(e)}")
        sys.exit(1)

    connections.append(conn)
    atexit.register(on_exit)

    test = api['TestCallset']

    reply = test.add(a=1000, b=2044)
    logger.info(f"result: sum={reply.result.sum}")

    reply = test.handlererror()
    logger.info(f"result={reply.result}")

    reply = test.setstruct(
        var_int32=-55,
        var_uint32=40001230,
        var_int64=0x0000beefbeefbeef,
        var_uint64=0x55,
        var_bool=True,
        var_uint32_array=[0, 1, 23, 4, 5, 6, 8, 123456789],
        var_string="hello world, th",
        var_bytes=b'asdfjkl;',
        no_reply=False
    )
    logger.info(f"result={reply.result}")


def main(ip):

    pass


def entrypoint():
    cli(obj={})


if __name__ == "__main__":
    main()
