import sys
import logging
from rich.logging import RichHandler
from rich import inspect

from protorpc import build_api

# Import callset classes
from proto.TestRpc.lib.test import TestCallset
from proto.ProtoRpc.lib import ProtoRpcHeader

logger = logging.getLogger()


def main(ip):

    api, conn = build_api(ProtoRpcHeader,
                          [(TestCallset, 1)],
                          port=13001,
                          protocol='tcp',
                          addr=ip)

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


if __name__ == "__main__":
    """ Usage: python -m app.test <ip>
    """
    logger.setLevel(logging.INFO)
    ch = RichHandler(rich_tracebacks=True, show_time=False)
    ch.setLevel(logging.DEBUG)
    logger.addHandler(ch)

    main(sys.argv[1])
