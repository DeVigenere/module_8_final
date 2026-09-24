# module_8_final

A C++17 system that transmits events via TCP, validates them, saves them to SQLite, and serves them upon request via a control channel. The network layer is abstracted and implemented for both UNIX and Windows systems. The database is represented by a repository interface (`IDataBase`).

## Features

- **Network event exchange.** The sender (`calculator_sender`) generates JSON messages containing a UTC timestamp, status, and payload; the receiver reads the stream line-by-line and responds with "Ok" or "Error".
- **Strict input validation.** Checks are performed for data types, ISO-8601 UTC time format, valid status values, and an allowed list of `source_service` values ​​(`calculator`, `ipv4_filter`, `control`). Invalid JSON or fields do not crash the process; an error is returned instead.
- **Persistence.** Events are written to `messages.db` (using SQLite). Data remains intact after a restart.
- **Control channel.** Events with `source_service: control` are routed to `ControlService`, where commands are registered by name:
- `stats` — statistics (total, success, error, by_source); 
- `get_events` — data retrieval with filters for `from`/`to`/`limit`/`status`/`source_service`.
- **Error handling.** Division by zero in the Calculator results in an `invalid_argument` exception; the receiver sends an event with `status: "error"` and an `error` field in the payload.
- **Tests.** A GoogleTest suite covers the calculator, message parser, `ControlService`, and `SqliteDatabase` (including filters and limits). ## Message Contract

json
{
"source_service": "calculator",
"timestamp_utc": "2025-01-01T12:00:00.123Z",
"payload": { "a": 10, "b": 5, "operation": "+", "result": 15 },
"status": "success",
"schema_version": 1
}

## CI
GitHub Actions (`.github/workflows/build.yml`) builds the project and runs tests on
`ubuntu-22.04` and `windows-2022` using the vcpkg cache. After the build, it verifies the
presence of the `receiver` and `calculator_sender` artifacts in the `bin` directory.

## Build

The project automatically downloads all necessary frameworks; these dependencies are defined in `vcpkg.json`.

CMake 3.20

Open a command prompt in the project directory:

- `mkdir build && cd build` (creates the `build` folder and switches to it)

- `cmake ..` (prepares CMake)

- `cmake --build . --config Release` (starts the build)

- A `bin` folder will appear in the project directory; enter it.

- Then enter the `Release` folder.

- Run `receiver.exe` first, followed by `calculator_sender.exe`.

- Once the transmission completes and displays the delivery count (e.g., "X of Y messages delivered"), a `messages.db` file will appear in the `db` folder; this contains the database.
(Open it however you prefer; for quick debugging, I used an online SQLite viewer project from GitHub.)

- (Optional) If you want to run tests, there is a `unit_tests.exe` file in the `Release` folder—run that.

- If you wish to modify the messages being sent, you can change the `ops` in `Calculator_sender.cpp` before building.


# module_8_final

Система на C++17, которая шлёт события по TCP и валидирует их, затем
сохраняет в SQLite и отдаёт по запросу через control-канал. Сетевой слой абстрагирован
и реализован под UNIX и Windows - системы. База данных
представлена интерфейсом репозитори (IDataBase) 

## Возможности

- **Обмен событиями по сети.** Отправитель(calculator_sender) формирует JSON-сообщения с UTC-таймштампом, статусом
  и payload; получатель(receiver) читает поток построчно и отвечает Ok or Error.
- **Строгая валидация входа.** Проверяются типы, ISO-8601 UTC-формат времени, допустимые значения
  status и белый список source_service (calculator, ipv4_filter, control). Невалидный
  JSON или поля не роняют процесс — возвращается ошибка.
- **Долговременность.** События пишутся в messages.db (в моём случае в SQLite). После перезапуска данные
  остаются на месте.
- **Control-канал.** События с source_service: control маршрутизируются в ControlService,
  где команды регистрируются по имени:
  - stats — статистика (total, success, error, by_source);
  - get_events — выборка с фильтрами from/to/limit/status/source_service.
- **Обработка ошибок.** Деление на ноль в Calculator даёт invalid_argument,
  отправитель(receiver) шлёт событие со status: "error" и полем error в payload.
- **Тесты.** GoogleTest-набор покрывает калькулятор, парсер сообщений, ControlService
  и SqliteDatabase (в т.ч. фильтры и лимиты).

## Контракт сообщения

json
{
  "source_service": "calculator",
  "timestamp_utc": "2025-01-01T12:00:00.123Z",
  "payload": { "a": 10, "b": 5, "operation": "+", "result": 15 },
  "status": "success",
  "schema_version": 1
}

## CI
GitHub Actions (.github/workflows/build.yml) собирает проект и прогоняет тесты на
ubuntu-22.04 и windows-2022 с кэшем vcpkg. После сборки проверяется наличие артефактов
receiver и calculator_sender в bin

## Сборка

Все нужные фреймы проект докачает сам, эти зависимости описаны в vkpcg.json

CMake 3.20

cmd на папку, где лежит проект()

- mkdir build && cd build (создаст папку build и cmd передет в нее)

- cmake .. (подготовка cmake)

- cmake --build . --config Release (начинает сборку)

- Далее в папке с проектом появится папка bin - заходим в неё

- Далее в папку Release

- Далее запускаем сначала receiver.exe, затем calculator_sender.exe

- После того, как в конце отправки написало сколько сообщений из скольки доставлено в папке db появится файл messages.db, в нём и будет база
(открывайте любым удобным способом, я для быстроты отладки пользовался сторонним проектом на гитхабе SQLITE Viewer online)

- (Неважно). Если хотите запустить тесты, то в папке Release есть ещё unit_tests.exe - запускайте его.

- Если вы хотите изменить сообщения для отправки, то можете поменять ops в Calculator_sender.cpp ещё до сборки 
