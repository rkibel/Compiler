import json

with open('ast.json', 'r+') as f:
    data = json.load(f)
    f.seek(0)
    json.dump(data, f, indent=2)
    f.truncate()
    
with open('lir.json', 'r+') as f:
    data = json.load(f)
    f.seek(0)
    json.dump(data, f, indent=2)
    f.truncate()