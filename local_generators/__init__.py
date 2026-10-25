"""Generators of the private, original-derived build inputs (decision D13).

Every generator reads the user's own hash-pinned swkotor.exe through
original.OriginalImage and writes into a fresh output directory that must
never be committed or published. generate_local_resources.py runs them all.
"""
