import json

# Шляхи до файлів
f1 = 'data/etryvoga.json'
f2 = 'data/ua_districts_and_cities_with_id_full.json'

# Збір усіх slug з districts.json (райони + міста)
def collect_slugs(obj):
    slugs = set()
    if isinstance(obj, dict):
        if 'slug' in obj and isinstance(obj['slug'], str):
            slugs.add(obj['slug'])
        if 'districts' in obj and isinstance(obj['districts'], list):
            for d in obj['districts']:
                slugs |= collect_slugs(d)
        if 'cities' in obj and isinstance(obj['cities'], list):
            for c in obj['cities']:
                slugs |= collect_slugs(c)
    elif isinstance(obj, list):
        for item in obj:
            slugs |= collect_slugs(item)
    return slugs

with open(f1, encoding='utf-8') as f:
    d1 = json.load(f)
slugs1 = collect_slugs(d1)

# Збір усіх slug з ua_districts_and_cities_with_id_full.json
with open(f2, encoding='utf-8') as f:
    d2 = json.load(f)
slugs2 = set()
for item in d2:
    if 'slug' in item and isinstance(item['slug'], str):
        slugs2.add(item['slug'])

# Різниця
missing = sorted(slugs1 - slugs2)
print('Відсутні slug у ua_districts_and_cities_with_id_full.json:')
for s in missing:
    print(s)
