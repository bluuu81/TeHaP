import psycopg2

# Połącz z PostgreSQL
conn = psycopg2.connect(
    dbname="thpbase",
    user="postgres",
    password="pass", #ChangeMe
    host="localhost"
)
conn.autocommit = True

# Utwórz kursor
cur = conn.cursor()

# Utwórz tabele
tables = [
    """
    CREATE TABLE IF NOT EXISTS public.devices (
        uid INTEGER PRIMARY KEY,
        device_name TEXT NOT NULL,
        remarks TEXT
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS public.status_data (
        id SERIAL PRIMARY KEY,
        uid INTEGER REFERENCES public.devices(uid),
        mcu_temp REAL NOT NULL,
        vbat INTEGER NOT NULL,
        vac1 INTEGER NOT NULL,
        vac2 INTEGER NOT NULL,
        chrg_status INTEGER NOT NULL,
        timestamp INTEGER NOT NULL,
        crc_ok BOOLEAN NOT NULL
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS public.localize_data (
        id SERIAL PRIMARY KEY,
        uid INTEGER REFERENCES public.devices(uid),
        lat REAL NOT NULL,
        lon REAL NOT NULL,
        satnum INTEGER NOT NULL,
        fixtype INTEGER NOT NULL,
        timestamp INTEGER NOT NULL,
        crc_ok BOOLEAN NOT NULL
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS public.temp_data (
        id SERIAL PRIMARY KEY,
        uid INTEGER REFERENCES public.devices(uid),
        value1 REAL NOT NULL,
        value2 REAL NOT NULL,
        value3 REAL NOT NULL,
        timestamp INTEGER NOT NULL,
        crc_ok BOOLEAN NOT NULL
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS public.press_data (
        id SERIAL PRIMARY KEY,
        uid INTEGER REFERENCES public.devices(uid),
        value1 REAL NOT NULL,
        value2 REAL NOT NULL,
        value3 REAL NOT NULL,
        timestamp INTEGER NOT NULL,
        crc_ok BOOLEAN NOT NULL
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS public.hum_data (
        id SERIAL PRIMARY KEY,
        uid INTEGER REFERENCES public.devices(uid),
        value1 REAL NOT NULL,
        value2 REAL NOT NULL,
        value3 REAL NOT NULL,
        timestamp INTEGER NOT NULL,
        crc_ok BOOLEAN NOT NULL
    )
    """
]

for table in tables:
    cur.execute(table)

# Zamknij połączenie
cur.close()
conn.close()
