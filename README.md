# int-stack-chardev

Advanced Linux lab 4 (int_stack chardev)

## Обзор

Модуль ядра `int_stack` создает символьное устройство (`/dev/int_stack`), которое ведет себя как структура данных стека целых чисел.

## Сборка и загрузка / выгрузка

### Сборка модуля
```bash
make
```

### Загрузка модуля
```bash
sudo insmod int_stack.ko
```

### Проверка
Можно проверить успешно ли загружен модуль:

1. Через список модулей:
    ```bash
    lsmod | grep int_stack
    ```

2. Через системные журналы:
    ```bash
    sudo dmesg | tail
    ```

3. Через файл устройства:
    ```bash
    ls -l /dev/int_stack
    ```

### Установка прав доступа
По умолчанию файл устройства создается с доступом только для root. Чтобы использовать утилиту без sudo нужно изменить права доступа:
```bash
sudo chmod 666 /dev/int_stack
```

### Выгрузка модуля
```bash
sudo rmmod int_stack
```

## Утилита для работы со стеком

Утилита `kernel_stack.py` предоставляет CLI для взаимодействия со стеком:

- `python3 kernel_stack.py set-size <size>` - Установить емкость стека
- `python3 kernel_stack.py push <value>` - Добавить значение в стек
- `python3 kernel_stack.py pop` - Извлечь значение из стека
- `python3 kernel_stack.py unwind` - Извлечь все элементы из стека
