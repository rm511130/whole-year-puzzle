# whole-year-puzzle
Fast solution (solver.c) to the whole-year-puzzle written in C

![image](https://github.com/rm511130/whole-year-puzzle/assets/11321060/0220edc1-7872-4463-ac79-07c74f28ed59)

# Grab solver.c and compile it

`$ gcc -O3 -march=native -flto solver.c -o solver`

or just

`$ gcc solver.c -o solver`

# Run it
```
$ ./solver May 17

May 17

   3 4 5 6 7 8 9
 3 5 5 8 8 X 1 X
 4 5 5 8 1 1 1 X
 5 0 0 8 1 4 4 4
 6 0 3 3 3 4 4 7
 7 0 3 X 3 2 7 7
 8 0 6 6 2 2 2 7
 9 X X 6 6 2 X X

Time taken to execute: 0.437500 seconds
```



# Run it for every day of the year

```
#!/bin/bash

# Define arrays for months and days in each month
months=("Jan" "Feb" "Mar" "Apr" "May" "Jun" "Jul" "Aug" "Sep" "Oct" "Nov" "Dec")
days_in_month=(31 28 31 30 31 30 31 31 30 31 30 31)

# Loop through each month
for i in ${!months[@]}; do
    month=${months[$i]}
    days=${days_in_month[$i]}
    
    # Check for leap year if it's February
    if [ "$month" == "Feb" ]; then
        if (( $(date +%Y) % 4 == 0 && ( $(date +%Y) % 100 != 0 || $(date +%Y) % 400 == 0 ) )); then
            days=29
        fi
    fi
    
    # Loop through each day of the month
    for (( day=1; day<=days; day++ )); do
        ./solver $month $day
    done
done
```
