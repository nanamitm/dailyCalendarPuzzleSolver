import ast
import contextlib
import io
from pathlib import Path
import unittest

from puzzle import Board, Piece, Vector
from solver import PuzzleSolver
from multithreadssolver import MultiThreadPuzzleSolver


def board(width):
    return Board([[0] * (width + 4), [0] * (width + 4),
                  [0, 0] + [None] * width + [0, 0],
                  [0] * (width + 4), [0] * (width + 4)])


class SolverRegressions(unittest.TestCase):
    def solve_quietly(self, solver, **kwargs):
        with contextlib.redirect_stdout(io.StringIO()):
            return solver.solve(printSol=False, **kwargs)

    def test_all_python_sources_parse(self):
        for path in Path(__file__).parent.glob('*.py'):
            with self.subTest(path=path.name):
                ast.parse(path.read_text(encoding='utf-8'), filename=str(path))

    def test_reuse_resets_stop_and_counts(self):
        solver = PuzzleSolver(board(2), [Piece([Vector(1, 0)], 'A')])
        first = self.solve_quietly(solver)
        second = self.solve_quietly(solver)
        self.assertEqual(len(first[0]), 1)
        self.assertEqual(len(second[0]), 1)
        self.assertEqual(first[1:], second[1:])

    def test_back_side_handles_symmetric_and_single_cell_pieces(self):
        for solver_class in (PuzzleSolver, MultiThreadPuzzleSolver):
            for width in (1, 2):
                with self.subTest(solver=solver_class.__name__, width=width):
                    shape = [Vector(1, 0)] if width == 2 else []
                    solver = solver_class(board(width), [Piece(shape, 'A')])
                    result = self.solve_quietly(solver, sides='back', findAll=True)
                    self.assertEqual(len(result[0]), 1)

    def test_rejects_underfilled_and_overfilled_sets(self):
        for solver_class in (PuzzleSolver, MultiThreadPuzzleSolver):
            for width in (1, 3):
                with self.subTest(solver=solver_class.__name__, width=width):
                    solver = solver_class(board(width), [Piece([Vector(1, 0)], 'A')])
                    self.assertEqual(self.solve_quietly(solver), ([], 0, 0))


if __name__ == '__main__':
    unittest.main()
