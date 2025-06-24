echo "the running file: $0"
commit_log=$1
current_date=$(date +'%Y-%m-%d')
git add .
git commit -m "$commit_log-$current_date"
git push origin main