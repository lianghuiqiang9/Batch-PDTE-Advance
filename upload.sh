echo "the running file: $0"
commit_log=$1
git add .
git commit -m "$commit_log"
git push origin main