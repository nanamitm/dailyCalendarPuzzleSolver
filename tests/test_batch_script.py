import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


class BatchScriptTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bash = (r'C:\Program Files\Git\bin\bash.exe' if os.name == 'nt'
                    else shutil.which('bash'))
        if not cls.bash or not Path(cls.bash).is_file():
            raise unittest.SkipTest('Bash is unavailable')

    def test_all_months_and_json_option(self):
        source = Path(__file__).resolve().parents[1] / 'cpp' / 'solveAllDates.sh'
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            tools = root / 'tools with spaces'
            tools.mkdir()
            script = tools / source.name
            script.write_text(source.read_text(), encoding='utf-8', newline='\n')
            solver = tools / 'poodlepuzzleDailyCalendarSolver.bin'
            solver.write_text(
                '#!/bin/bash\n'
                '[[ $# == 4 && $4 == -i ]] || exit 2\n'
                'printf \'"%s-%s-%s": {"nbSol":0,"sols":[]}\' "$1" "$2" "$3"\n'
                'exit 1\n', encoding='utf-8', newline='\n')
            solver.chmod(0o755)
            result = subprocess.run([self.bash, str(script)], cwd=root,
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stderr)
            for month in range(1, 13):
                entries = json.loads((root / f'Month_{month}.txt').read_text())
                self.assertEqual(len(entries), 7 * 31)
                self.assertIn(f'7-31-{month}', entries)
            invalid = subprocess.run([self.bash, str(script), '13'], cwd=root,
                                     capture_output=True, timeout=10)
            self.assertEqual(invalid.returncode, 2)

    def test_solver_argument_error_aborts_export(self):
        source = Path(__file__).resolve().parents[1] / 'cpp' / 'solveAllDates.sh'
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            script = root / source.name
            script.write_text(source.read_text(), encoding='utf-8', newline='\n')
            # The solver exits 2 and prints its help on invalid arguments;
            # that must stop the export instead of being stored as "no solution".
            solver = root / 'poodlepuzzleDailyCalendarSolver.bin'
            solver.write_text('#!/bin/bash\necho "Synopsis:"\nexit 2\n',
                              encoding='utf-8', newline='\n')
            solver.chmod(0o755)
            result = subprocess.run([self.bash, str(script), '1'], cwd=root,
                                    capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, 2)
            self.assertNotIn('Synopsis', (root / 'Month_1.txt').read_text())


if __name__ == '__main__':
    unittest.main()
