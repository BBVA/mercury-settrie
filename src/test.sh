coverage run --omit /usr/lib/*,settrie/__init__.py -m pytest test_all.py
coverage report -m
