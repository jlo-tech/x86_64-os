#include <sync.h>

void mutex_init(mutex_t *mtx)
{
    *mtx = 0;
}

void mutex_lock(mutex_t *mtx)
{
    while(atomic_tas(mtx));
}

void mutex_unlock(mutex_t *mtx)
{
    atomic_clr(mtx);
}

void semaphore_init(semaphore_t *sem, i64 val)
{
    sem->val = val;
    sem->lock = 0;
}

void semaphore_inc(semaphore_t *sem)
{
    mutex_lock(&sem->lock);
    sem->val++;
    mutex_unlock(&sem->lock);
}

void semaphore_dec(semaphore_t *sem)
{
    do
    {
        mutex_lock(&sem->lock);
        
        if(sem->val > 0)
            break;
        
        mutex_unlock(&sem->lock);
    } while(1);

    sem->val--;

    mutex_unlock(&sem->lock);
}
