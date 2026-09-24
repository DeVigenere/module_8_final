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
