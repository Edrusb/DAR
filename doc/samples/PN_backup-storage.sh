#!/bin/bash

dar -c "/mnt/storage/backup/storage_$(date +%Y-%m-%d-%H%M%S)" -B "/root/backup-storage.options"
