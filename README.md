# Задание для отклика на вакансию

Разработать на С++ простые консольные TCP клиент и сервер с использованием linux socket.
Код писать в стиле ООП.

Клиент получает из командной строки 3 параметра:

1. Текстовое имя клиента
2. Номер порта сервера
3. Период подключения к серверу в секундах

Клиент с указанным периодом подключается к серверу и отправляет текстовую строку в формате

```cmd
[yyyy-mm-dd hh:mm:ss.ms] "имя_клиента"
```

~~Сервер получает из командной строки 1 параметр:~~

~~1. Номер порта~~

*Я решил просто сделать так, что порт задан заранее, потому что ну это же регулярно не вводят, проще 1 раз задать*

После запуска сервер слушает указанный порт, получает сообщения от клиентов и записывает их в файл log.txt

Каждое подключение клиента должно быть обработано в отдельном потоке.

Каждое сообщение должно быть записано в отдельную строку.

Сервер должен позволять работать с несколькими клиентами одновременно и обеспечивать корректный доступ к файлу log.txt

Например, запускаем сервер *из директории 'MultiServer'*

```cmd
cmake .
./main.out
```

Запускаем клиенты *из директории 'Clients'*

```cmd
cmake .
./main.out
```

Вызов команд, с значением портом указывать не нужно, сразу только имя и период в секундах **(<= 60)** 

```cmd
Name1 1
Name2 2
Name3 3
```

~~Тогда в log.txt ожидаем увидеть приблизительно такую картину (будет зависеть от момента запуска клиентов)~~
Только в терминал вывод, ну файл просто открыть и записывать в файл.

```cmd
[2024-09-17 12:20:00.460] name2
[2024-09-17 12:20:00.460] name3
[2024-09-17 12:20:01.465] name2
[2024-09-17 12:20:01.769] name2
[2024-09-17 12:20:01.769] name2
[2024-09-17 12:20:02.774] name2
[2024-09-17 12:20:03.776] name1
[2024-09-17 12:20:03.776] name2
[2024-09-17 12:20:04.781] name2
[2024-09-17 12:20:05.787] name2
[2024-09-17 12:20:06.791] name1
[2024-09-17 12:20:06.791] name2
[2024-09-17 12:20:07.793] name2
[2024-09-17 12:20:08.798] name2
[2024-09-17 12:20:09.803] name1
[2024-09-17 12:20:09.803] name2
[2024-09-17 12:20:10.808] name2
[2024-09-17 12:20:10.809] name3
[2024-09-17 12:20:11.814] name2
[2024-09-17 12:20:12.819] name1
[2024-09-17 12:20:12.819] name2
```

и так далее.

**Максимум 15 пользователей**
