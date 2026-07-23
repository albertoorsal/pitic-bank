/* user_repository.c - Dispatch helpers shared by every repository backend. */
#include "user_repository.h"


RepoStatus repo_create(const UserRepository *repo, const User *input, User *created){
    return repo->create(repo->self, input, created);
}
 
RepoStatus repo_find_by_id(const UserRepository *repo, long id, User *out){
    return repo->find_by_id(repo->self, id, out);
}

RepoStatus repo_find_by_email(const UserRepository *repo, const char *email, User *out){
    return repo->find_by_email(repo->self, email, out);
}

RepoStatus repo_verify_credentials(const UserRepository *repo, const char *email, const char *password, User *out){
    return repo->verify_credentials(repo->self, email, password, out);
}

RepoStatus repo_find_all(const UserRepository *repo, UserList *out){
    return repo->find_all(repo->self, out);
}
 
RepoStatus repo_update(const UserRepository *repo, const User *input){
    return repo->update(repo->self, input);
}
 
RepoStatus repo_remove(const UserRepository *repo, long id){
    return repo->remove(repo->self, id);
}

void repo_destroy(UserRepository *repo) {
    if (repo != NULL && repo->destroy != NULL) {
        return repo->destroy(repo->self);
    }
}