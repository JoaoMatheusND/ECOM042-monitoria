"""Regressao de linhas grandes na UART do native_sim (Linux/WSL)."""

import os
from pathlib import Path
import pty
import select
import subprocess
import sys
import tempfile
import time
import tty
import unittest


EXECUTABLE = Path(sys.argv.pop(1)).resolve()


class UartRecovery(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.master, cls.slave = pty.openpty()
        tty.setraw(cls.master)
        tty.setraw(cls.slave)
        cls.boot = tempfile.TemporaryFile()
        cls.process = subprocess.Popen(
            [str(EXECUTABLE), '-rt', f'-uart_1_port={os.ttyname(cls.slave)}'],
            stdout=cls.boot, stderr=cls.boot,
        )
        cls.read_prompts(1, initial=True)

    @classmethod
    def read_prompts(cls, count, initial=False):
        data = b''
        deadline = time.monotonic() + 10
        marker = b'> ' if initial else b'\r\n> '
        while time.monotonic() < deadline:
            if select.select([cls.master], [], [], 0.05)[0]:
                data += os.read(cls.master, 8192)
                if data.count(marker) >= count and data.endswith(b'> '):
                    return data
            if cls.process.poll() is not None:
                raise AssertionError('Firmware encerrou antes da resposta')
        raise AssertionError(f'Sem resposta completa: {data[-160:]!r}')

    def send(self, data, count=1):
        while data:
            size = os.write(self.master, data)
            data = data[size:]
        return self.read_prompts(count)

    def setUp(self):
        self.assertTrue(self.send(b'LED OFF\n').endswith(b'OK\r\n> '))

    def test_long_lines_keep_next_command(self):
        for size in (257, 1024, 4096):
            for end in (b'\n', b'\r', b'\r\n'):
                with self.subTest(size=size, end=end):
                    data = self.send(b'x' * size + end + b'LED\n', count=2)
                    self.assertEqual(data.count(b'ERR: line too long'), 1)
                    self.assertTrue(data.endswith(b'LED: OFF\r\n> '))

    def test_overflow_does_not_execute_prefix(self):
        data = self.send(b'LED ON' + b' ' * 1024 + b'\nLED\n', count=2)
        self.assertEqual(data.count(b'ERR: line too long'), 1)
        self.assertTrue(data.endswith(b'LED: OFF\r\n> '))

    @classmethod
    def tearDownClass(cls):
        cls.process.terminate()
        try:
            cls.process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            cls.process.kill()
            cls.process.wait()
        os.close(cls.master)
        os.close(cls.slave)
        cls.boot.close()


if __name__ == '__main__':
    unittest.main(verbosity=2)
