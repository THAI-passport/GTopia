import os
import sys
import glob
import shutil
import platform
import subprocess
import urllib.request
from pathlib import Path
from urllib.parse import urlparse

def print_success(msg): print(f"✅ {msg}")
def print_error(msg): print(f"❌ {msg}")
def print_warn(msg): print(f"⚠️ {msg}")
def print_info(msg): print(f"ℹ️ {msg}")

PROJECT_ROOT = Path(__file__).resolve().parent.parent

CERT_DIR = (PROJECT_ROOT / "HTTPServer").resolve()
RUNTIME_DIR = (PROJECT_ROOT / "Runtime").resolve()
CONFIGS_DIR = (PROJECT_ROOT / "Configs").resolve()
SQL_FILE = (CONFIGS_DIR / "gtopia.sql").resolve()
SQL_UPDATES_DIR = (CONFIGS_DIR / "Updates").resolve()

LAN_IP = "127.0.0.1"

sys.path.append(str(PROJECT_ROOT / "Util"))

try:
    from update_file_hashes import generate_file_hashes
    from generate_item_data import generate_item_txt_from_dat
    from generate_wiki_data import fetch_wiki_and_write
except ImportError as e:
    print_error(f"Required helper script missing or broken: {e}")
    print_info("Please ensure you cloned the repository completely.")
    sys.exit(1)

MKCERT_VERSION = "v1.4.4"
MKCERT_BASE = f"https://github.com/FiloSottile/mkcert/releases/download/{MKCERT_VERSION}"

IGNORABLE_SQL_ERRORS = ("ERROR 1050", "ERROR 1060", "ERROR 1061")

class DatabaseConfig:
    def __init__(self, name="gtopia", user="root", password=""):
        self.name = name
        self.user = user
        self.password = password

def run_silent(cmd, cwd=None):
    return subprocess.run(cmd, cwd=cwd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def run_powershell(command):
    try:
        result = subprocess.run(
            ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", command],
            capture_output=True, text=True, check=True
        )
        return True, result.stdout.strip()
    except subprocess.CalledProcessError as e:
        return False, (e.stderr or e.stdout or str(e)).strip()

def download_file(url, path):
    try:
        print_info(f"Downloading: {url}")
        urllib.request.urlretrieve(url, path)
        return True
    except Exception as e:
        print_error(f"Download failed: {e}")
        return False

def get_clean_input(prompt_text, default=None):
    suffix = f" [{default}]" if default is not None else ""
    user_input = input(f"{prompt_text}{suffix}: ").strip().strip('"').strip("'")
    return default if not user_input and default is not None else user_input

def ask_yes_no(prompt_text, default="y"):
    hint = "Y/n" if default == "y" else "y/N"
    return get_clean_input(f"\n👉 {prompt_text} [{hint}]", default=default).lower() == "y"

def get_valid_path(prompt_text, is_file=False, is_dir=False):
    while True:
        path_str = get_clean_input(prompt_text)
        if not path_str:
            print_warn("Path cannot be empty.")
        else:
            path = Path(path_str).expanduser().resolve()
            if is_file and not path.is_file():
                print_error(f"Not a valid file: {path}")
            elif is_dir and not path.is_dir():
                print_error(f"Not a valid folder: {path}")
            else:
                return path

        if get_clean_input("👉 Type 'S' to skip, or press Enter to try again", default="").lower() == "s":
            return None

def move_file(src, dst_dir, overwrite=False):
    src, dst_dir = Path(src), Path(dst_dir)
    if not src.exists():
        print_error(f"File not found: {src.name}")
        return False

    dst_dir.mkdir(parents=True, exist_ok=True)
    dst = dst_dir / src.name

    if dst.exists():
        if not overwrite:
            print_warn(f"Skipped (already exists): {dst.name}")
            return False
        dst.unlink()

    try:
        shutil.move(str(src), str(dst))
        print_success(f"Moved: {src.name} -> {dst_dir.name}/")
        return True
    except Exception as e:
        print_error(f"Failed to move {src.name}: {e}")
        return False

def check_cmake():
    print_info("Checking CMake installation...")
    if shutil.which("cmake"):
        print_success("CMake is ready.")
        return

    print_error("CMake was not found on your system PATH.")
    if platform.system() == "Windows":
        print_info("Install CMake from: https://cmake.org/download/")
        print_warn("Check 'Add CMake to the system PATH' during installation, then restart this script.")
    else:
        print_info("Install via your package manager: sudo apt install cmake")
    sys.exit(1)

def check_mysql():
    print_info("Checking MySQL installation...")
    if platform.system() == "Windows":
        patterns = [
            r"C:\Program Files\MySQL\MySQL Server *\bin\mysql.exe",
            r"C:\Program Files (x86)\MySQL\MySQL Server *\bin\mysql.exe"
        ]
        for pattern in patterns:
            matches = sorted(glob.glob(pattern))
            if matches:
                print_success(f"MySQL found: {matches[-1]}")
                return matches[-1]

    mysql = shutil.which("mysql")
    if mysql:
        print_success(f"MySQL found: {mysql}")
        return mysql

    print_error("MySQL client (mysql) could not be found.")
    if platform.system() == "Windows":
        print_info("Download: https://dev.mysql.com/downloads/installer/")
    else:
        print_info("Install via package manager: sudo apt install mysql-client libmysql-dev")
    sys.exit(1)

def task_generate_items():
    print("\n--- [Item Data Generator] ---")
    print_info("Needs the items.dat file from your Growtopia game folder (Growtopia/cache/items.dat).")
    dat_path = get_valid_path("Path to items.dat", is_file=True)
    if dat_path:
        print_info("Generating items.txt...")
        generate_item_txt_from_dat(0, dat_path)
        move_file(PROJECT_ROOT / "items.txt", RUNTIME_DIR, overwrite=True)

def task_generate_wiki():
    print("\n--- [Wiki Data Generator] ---")
    print_info("Needs the same items.dat file. This downloads wiki info, it can take a while.")
    dat_path = get_valid_path("Path to items.dat", is_file=True)
    if dat_path:
        print_info("Processing wiki data...")
        fetch_wiki_and_write(0, dat_path)
        move_file(PROJECT_ROOT / "wiki_data.txt", RUNTIME_DIR, overwrite=True)

def task_generate_hashes():
    print("\n--- [File Hash Generator] ---")
    print_info("Pick the static folder that contains 'audio', 'interface' and 'game' subfolders.")
    static_path = get_valid_path("Static folder path", is_dir=True)
    if static_path:
        print_info("Processing file hashes, it might take a bit...")
        generate_file_hashes(static_path)
        move_file(PROJECT_ROOT / "filehashes.txt", RUNTIME_DIR, overwrite=True)

def get_mkcert_url():
    sys_type = platform.system().lower()
    arch = platform.machine().lower()

    os_info = {
        "windows": {"amd64": "windows-amd64.exe", "x86_64": "windows-amd64.exe", "arm64": "windows-arm64.exe"},
        "linux": {"x86_64": "linux-amd64", "amd64": "linux-amd64", "aarch64": "linux-arm64", "arm64": "linux-arm64"},
        "darwin": {"arm64": "darwin-arm64", "amd64": "darwin-amd64", "x86_64": "darwin-amd64"}
    }

    suffix = os_info.get(sys_type, {}).get(arch)
    if not suffix:
        raise Exception(f"Unsupported platform: {sys_type}-{arch}")
    return f"{MKCERT_BASE}/mkcert-{MKCERT_VERSION}-{suffix}"

def setup_ssl_certificates():
    print_info("Generating trusted local SSL certificates...")
    exe_name = "mkcert.exe" if platform.system() == "Windows" else "mkcert"
    mkcert_path = PROJECT_ROOT / exe_name

    if not mkcert_path.exists():
        try:
            url = get_mkcert_url()
        except Exception as e:
            print_error(str(e))
            return
        if not download_file(url, mkcert_path):
            return
        if platform.system() != "Windows":
            os.chmod(str(mkcert_path), 0o755)

    print_info("Installing root certificate (you may see a UAC / password prompt)...")
    if run_silent([str(mkcert_path), "-install"], cwd=PROJECT_ROOT).returncode != 0:
        print_warn("mkcert -install reported a problem, the certificate may not be trusted.")
    if run_silent([str(mkcert_path), "*.growtopia1.com", "*.growtopia2.com"], cwd=PROJECT_ROOT).returncode != 0:
        print_error("Failed to generate certificates.")
        return

    CERT_DIR.mkdir(parents=True, exist_ok=True)
    for target_pem in glob.glob(str(PROJECT_ROOT / "*.pem")):
        dest_name = "key.pem" if "-key" in os.path.basename(target_pem) else "cert.pem"
        dest_path = CERT_DIR / dest_name
        if dest_path.exists():
            dest_path.unlink()
        shutil.move(target_pem, str(dest_path))
        print_success(f"Deployed certificate: {dest_name}")

def get_latest_growtopia_cdn():
    print_info("Fetching latest Ubisoft CDN...")
    url = "https://growtopiagame.com/Growtopia-Installer.exe"
    try:
        with urllib.request.urlopen(urllib.request.Request(url, method="HEAD"), timeout=15) as res:
            final_url = res.geturl()
    except Exception as e:
        print_error(f"Connection error while fetching CDN: {e}")
        return ""

    if "akamaihd.net" not in final_url:
        return ""

    parsed = urlparse(final_url)
    cdn_pathname = parsed.path.replace("GrowtopiaInstaller.exe", "")
    return f"{parsed.hostname}{cdn_pathname}cache/"

def run_mysql_query(mysql_client, args, sql_input):
    try:
        return subprocess.run(
            [mysql_client] + args, input=sql_input, text=True,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
    except Exception as e:
        print_error(f"Could not run MySQL: {e}")
        return None

def mysql_auth(user, password):
    args = ["-u", user]
    if password:
        args.append(f"-p{password}")
    return args

def mysql_error(result):
    if not result:
        return "Process failure"
    lines = [l for l in result.stderr.splitlines() if "[Warning]" not in l]
    return "\n".join(lines).strip()

def migration_ok(result):
    if not result:
        return False
    if result.returncode == 0:
        return True
    errors = [l for l in result.stderr.splitlines() if "ERROR" in l]
    return bool(errors) and all(any(code in l for code in IGNORABLE_SQL_ERRORS) for l in errors)

def get_existing_databases(mysql_client, user, password):
    res = run_mysql_query(mysql_client, mysql_auth(user, password), "SHOW DATABASES;")
    if res and res.returncode == 0 and res.stdout:
        system_dbs = {"information_schema", "performance_schema", "mysql", "sys"}
        return [
            line.strip() for line in res.stdout.strip().splitlines()[1:]
            if line.strip() and line.strip().lower() not in system_dbs
        ]
    return []

def init_schema_migrations(mysql_client, auth_args, db_name):
    sql = f"""
    USE `{db_name}`;
    CREATE TABLE IF NOT EXISTS `SchemaMigrations` (
      `Version` VARCHAR(255) PRIMARY KEY,
      `ApplyTime` TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    """
    res = run_mysql_query(mysql_client, auth_args, sql)
    return bool(res and res.returncode == 0)

def update_database_tables(mysql_client, cfg):
    auth_args = mysql_auth(cfg.user, cfg.password)

    res = run_mysql_query(mysql_client, auth_args, f"CREATE DATABASE IF NOT EXISTS `{cfg.name}`;")
    if not res or res.returncode != 0:
        print_error(f"Cannot connect to MySQL:\n{mysql_error(res)}")
        return False, []

    if not init_schema_migrations(mysql_client, auth_args, cfg.name):
        print_error("Could not create the SchemaMigrations table.")
        return False, []

    res = run_mysql_query(mysql_client, auth_args, f"USE `{cfg.name}`; SELECT `Version` FROM `SchemaMigrations`;")
    if not res or res.returncode != 0:
        print_error(f"Could not read applied migrations:\n{mysql_error(res)}")
        return False, []
    applied_versions = set(line.strip() for line in res.stdout.strip().splitlines()[1:] if line.strip())

    SQL_UPDATES_DIR.mkdir(parents=True, exist_ok=True)
    pending = [f for f in sorted(SQL_UPDATES_DIR.glob("*.sql")) if f.name not in applied_versions]

    applied_now = []
    for sql_file in pending:
        print_info(f"Applying migration: {sql_file.name}...")
        sql_content = sql_file.read_text(encoding="utf-8-sig")
        result = run_mysql_query(
            mysql_client,
            auth_args + ["--force", "--default-character-set=utf8mb4", cfg.name],
            sql_content
        )

        if not migration_ok(result):
            print_error(f"Failed to apply {sql_file.name}:\n{mysql_error(result)}")
            return False, applied_now

        record = run_mysql_query(
            mysql_client, auth_args,
            f"USE `{cfg.name}`; INSERT IGNORE INTO `SchemaMigrations` (`Version`) VALUES ('{sql_file.name}');"
        )
        if not record or record.returncode != 0:
            print_error(f"Applied {sql_file.name} but could not record it:\n{mysql_error(record)}")
            return False, applied_now

        applied_now.append(sql_file.name)
        print_success(f"Applied: {sql_file.name}")

    return True, applied_now

def report_updates(success, applied):
    if not success:
        print_error("Database update stopped because of an error (see above).")
    elif applied:
        print_success(f"Applied {len(applied)} migration(s): {', '.join(applied)}")
    else:
        print_info("Database is already up to date.")

def run_update_sql_tables():
    print("\n--- [Update SQL Tables] ---")
    print_info(f"Runs every new .sql file from: {SQL_UPDATES_DIR}")
    mysql_client = check_mysql()

    user = get_clean_input("MySQL username", default="root")
    password = get_clean_input("MySQL password (leave empty if none)", default="")

    dbs = get_existing_databases(mysql_client, user, password)
    if dbs:
        print_info(f"Found databases: {', '.join(dbs)}")
        default_db = "gtopia" if "gtopia" in dbs else dbs[0]
    else:
        print_warn("Could not list databases (wrong login?). Continuing anyway.")
        default_db = "gtopia"

    db_name = get_clean_input("Database name to update", default=default_db)
    cfg = DatabaseConfig(db_name, user, password)
    success, applied = update_database_tables(mysql_client, cfg)
    report_updates(success, applied)

def run_database_wizard(mysql_client):
    print("\n--- MySQL Database Wizard ---")
    print_info("Creates the database and imports Configs/gtopia.sql. Press Enter to accept [defaults].")
    while True:
        cfg = DatabaseConfig(
            get_clean_input("Database name", default="gtopia"),
            get_clean_input("MySQL username", default="root"),
            get_clean_input("MySQL password (leave empty if none)", default="")
        )
        auth_args = mysql_auth(cfg.user, cfg.password)

        print_info(f"Connecting to MySQL and creating '{cfg.name}'...")
        result = run_mysql_query(mysql_client, auth_args, f"CREATE DATABASE IF NOT EXISTS `{cfg.name}`;")
        if not result or result.returncode != 0:
            print_error(f"Failed to connect or create the database:\n{mysql_error(result)}")
            if ask_yes_no("Re-enter credentials?"):
                continue
            return None

        print_success(f"Database '{cfg.name}' is ready.")
        if not SQL_FILE.exists():
            print_error(f"SQL file missing: {SQL_FILE}")
            return cfg

        print_info("Importing tables...")
        import_result = run_mysql_query(
            mysql_client,
            auth_args + ["--default-character-set=utf8mb4", cfg.name],
            SQL_FILE.read_text(encoding="utf-8-sig")
        )
        if not import_result or import_result.returncode != 0:
            print_error(f"Table import failed:\n{mysql_error(import_result)}")
            return cfg

        print_success(f"Tables imported into '{cfg.name}'.")
        success, applied = update_database_tables(mysql_client, cfg)
        report_updates(success, applied)
        return cfg

def update_config_line(file_path, prefix, replacement):
    if not file_path.exists():
        print_warn(f"Config file not found: {file_path.name}")
        return False
    lines = file_path.read_text(encoding="utf-8").splitlines(keepends=True)
    for i, line in enumerate(lines):
        if line.strip().startswith(prefix):
            lines[i] = replacement
            file_path.write_text("".join(lines), encoding="utf-8")
            return True
    print_warn(f"'{prefix}' not found in {file_path.name}, edit it manually.")
    return False

def configure_environment_firewalls(server_count):
    rules = [
        {"name": "GTopia-UDP-GamePorts", "proto": "UDP", "ports": f"18000-{18000 + server_count}"},
        {"name": "GTopia-TCP-SocketPorts", "proto": "TCP", "ports": f"18500-{18500 + server_count}"}
    ]
    print_info("Creating firewall inbound rules (run this script as Administrator)...")
    for rule in rules:
        ps_script = f'''
        Remove-NetFirewallRule -DisplayName "{rule['name']}" -ErrorAction SilentlyContinue
        New-NetFirewallRule -DisplayName "{rule['name']}" -Direction Inbound -Action Allow -Protocol {rule['proto']} -LocalPort "{rule['ports']}"
        '''
        ok, output = run_powershell(ps_script)
        if ok:
            print_success(f"Firewall rule: {rule['name']} -> {rule['proto']} ({rule['ports']})")
        else:
            print_error(f"Failed to create rule: {rule['name']}\n{output}")

def edit_configuration_files(db, latest_cdn):
    print("\n--------------------------------------------------")
    print("           Automatic Configuration                ")
    print("--------------------------------------------------")
    if not ask_yes_no("Write your settings into the config files automatically?"):
        print_info("Skipped. You will need to edit the config files in Runtime/ manually.")
        return

    print("\nWhere will the server run?")
    print("1) Local - only on this PC (127.0.0.1)")
    print("2) VPS/VDS - public server with its own IP")

    choice = get_clean_input("Choice [1-2]", default="1")
    if choice == "2":
        wan_ip = ""
        while not wan_ip:
            wan_ip = get_clean_input("Public IP address of your server")
    else:
        wan_ip = LAN_IP

    print_info("Applying changes...")

    cdn_host, cdn_path = latest_cdn.split("/", 1) if "/" in latest_cdn else ("", "")
    update_config_line(RUNTIME_DIR / "config.txt", "database_info|", f"database_info|localhost|{db.user}|{db.password}|{db.name}|3306|\n")
    if latest_cdn:
        update_config_line(RUNTIME_DIR / "config.txt", "cdn_server|", f"cdn_server|{cdn_host}|{cdn_path}|\n")

    (RUNTIME_DIR / "worlds").mkdir(parents=True, exist_ok=True)
    update_config_line(RUNTIME_DIR / "config.txt", "world_save_path|", f"world_save_path|{RUNTIME_DIR / 'worlds'}|\n")

    update_config_line(RUNTIME_DIR / "servers.txt", "set_master|", f"set_master|{LAN_IP}|{wan_ip}|\n")
    update_config_line(RUNTIME_DIR / "servers.txt", "add_server|", f"add_server|{LAN_IP}|{wan_ip}|1|\n")
    update_config_line(RUNTIME_DIR / "admin_server_config.txt", "admin_host|", f"admin_host|{LAN_IP}|\n")
    update_config_line(CERT_DIR / "main.go", "const SERVER_IP =", f'const SERVER_IP = "{wan_ip}"\n')

    print_success("Configuration done.")

    if platform.system() == "Windows" and choice == "2":
        if ask_yes_no("Create Windows Firewall rules for the game ports?"):
            try:
                count = int(get_clean_input("How many game servers will you run?", default="1"))
                configure_environment_firewalls(count)
            except ValueError:
                print_error("Please enter a number.")

def run_full_setup():
    print("==========================================")
    print("      GTopia Private Server Setup Wizard  ")
    print("==========================================")
    print_info("Answer Y (default) to each step. Press Enter to accept the value in [brackets].")
    print_info("Community: https://discord.gg/5XjTQm3kRh\n")

    check_cmake()
    mysql_client = check_mysql()

    db_config = DatabaseConfig()
    if ask_yes_no("Step 1: Create the database and import tables?"):
        wizard_result = run_database_wizard(mysql_client)
        if wizard_result:
            db_config = wizard_result

    if ask_yes_no("Step 2: Generate local SSL certificates? (needed for game version 3.90+)"):
        setup_ssl_certificates()

    RUNTIME_DIR.mkdir(parents=True, exist_ok=True)

    if ask_yes_no("Step 3: Generate items.txt? (required)"):
        task_generate_items()

    if ask_yes_no("Step 4: Generate filehashes.txt? (only for a custom CDN)", default="n"):
        task_generate_hashes()

    if ask_yes_no("Step 5: Generate wiki_data.txt?", default="n"):
        task_generate_wiki()

    print("\n--- Moving config files into Runtime/ ---")
    config_files = [
        "config.txt", "playmods.txt", "roles.txt", "admin_server_config.txt",
        "servers.txt", "achievements.txt", "store.txt", "consumable_data.txt", "battle_pet_data.txt", "news.txt"
    ]
    for config in config_files:
        move_file(CONFIGS_DIR / config, RUNTIME_DIR)

    (RUNTIME_DIR / "logs" / "crash").mkdir(parents=True, exist_ok=True)

    latest_cdn = get_latest_growtopia_cdn()
    edit_configuration_files(db_config, latest_cdn)

    print("\n==========================================")
    print("          SETUP COMPLETED                 ")
    print("==========================================\n")
    print_info(f"LAN IP: {LAN_IP}")
    if latest_cdn:
        print_info(f"Latest Ubisoft CDN: {latest_cdn}")

def run_get_latest_cdn():
    print("\n--- [Latest Ubisoft CDN] ---")
    cdn = get_latest_growtopia_cdn()
    if cdn:
        print_info(f"Active link found: {cdn}")
    else:
        print_error("Failed to fetch the latest Ubisoft CDN.")

def show_help():
    print("\n--- [Help] ---")
    print(" First time: choose [1] and answer the questions, defaults are fine.")
    print(" After pulling a new version: choose [6] to apply new database updates.")
    print(" After a Growtopia update: choose [3] (and [5] if you use a custom CDN).")
    print(" Needs: CMake and MySQL installed, MySQL running.")
    print(f" Database updates are read from: {SQL_UPDATES_DIR}")
    print(" Generated files and configs are placed in: " + str(RUNTIME_DIR))
    print(" Community: https://discord.gg/5XjTQm3kRh")

def main():
    menu_actions = {
        "1": run_full_setup,
        "2": run_get_latest_cdn,
        "3": task_generate_items,
        "4": task_generate_wiki,
        "5": task_generate_hashes,
        "6": run_update_sql_tables,
        "7": show_help
    }

    while True:
        subprocess.run("cls" if platform.system() == "Windows" else "clear", shell=True)

        print("\n==================================================")
        print("       GTOPIA PRIVATE SERVER SETUP PANEL          ")
        print("==================================================")
        print(" [1] Full setup (start here, first time)")
        print(" [2] Show latest Ubisoft CDN")
        print(" [3] Generate items.txt")
        print(" [4] Generate wiki_data.txt")
        print(" [5] Generate filehashes.txt")
        print(" [6] Update database (apply new SQL updates)")
        print(" [7] Help")
        print(" [0] Exit")
        print_info("Community: https://discord.gg/5XjTQm3kRh")

        choice = get_clean_input("Choice", default="1")
        if choice == "0":
            print_info("Exiting setup. Bye!")
            break

        action = menu_actions.get(choice)
        if action:
            action()
            print("\n--------------------------------------------------")
            input("👉 Press Enter to return to the Main Menu...")
        else:
            print_error("Invalid option.")
            input("👉 Press Enter to try again...")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print_error("\nSetup interrupted.")
        sys.exit(0)