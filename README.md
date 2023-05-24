# FTP Server

## Описание основного функционала FTP сервера

*File Transfer Protocol*

1. Создание хоста (сокета), подключение клиента/клиентов
2. Принятие и отправка сообщений клиенту
3. Обработка сообщений от клиента
4. Авторизация клиента
5. Создание директории для передаваемых файлов и серверных (лог с паролями и логинами) файлов
6. Передача файлов клиенту /// Принятие файлов от клиента

## Принимаемые команды

* -stop		***		Остановка сервера
* -help				Отображение описания команд
* -login			Авторизация на сервере
* -list				Отображение каталога файлов, загруженных на сервер
* -sendfile		Отправка файла с клиента на сервер
* -getfile		Отправка файла с сервера на клиент
