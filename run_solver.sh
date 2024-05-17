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
