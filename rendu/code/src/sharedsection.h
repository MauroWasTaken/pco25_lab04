//  /$$$$$$$   /$$$$$$   /$$$$$$         /$$$$$$   /$$$$$$   /$$$$$$  /$$$$$$$
// | $$__  $$ /$$__  $$ /$$__  $$       /$$__  $$ /$$$_  $$ /$$__  $$| $$____/
// | $$  \ $$| $$  \__/| $$  \ $$      |__/  \ $$| $$$$\ $$|__/  \ $$| $$
// | $$$$$$$/| $$      | $$  | $$        /$$$$$$/| $$ $$ $$  /$$$$$$/| $$$$$$$
// | $$____/ | $$      | $$  | $$       /$$____/ | $$\ $$$$ /$$____/ |_____  $$
// | $$      | $$    $$| $$  | $$      | $$      | $$ \ $$$| $$       /$$  \ $$
// | $$      |  $$$$$$/|  $$$$$$/      | $$$$$$$$|  $$$$$$/| $$$$$$$$|  $$$$$$/
// |__/       \______/  \______/       |________/ \______/ |________/ \______/


#ifndef SHAREDSECTION_H
#define SHAREDSECTION_H

#include <atomic>
#include <QDebug>
#include <gtest/internal/gtest-port.h>

#include <pcosynchro/pcosemaphore.h>

#ifdef USE_FAKE_LOCO
#  include "fake_locomotive.h"
#else
#  include "locomotive.h"
#endif

#ifndef USE_FAKE_LOCO
#include "ctrain_handler.h"
#endif

#include "sharedsectioninterface.h"

/**
 * @brief La classe SharedSection implémente l'interface SharedSectionInterface qui
 * propose les méthodes liées à la section partagée.
 */
class SharedSection final : public SharedSectionInterface {
public:

    // fonction que l'on attend a la prochaine fonction
    enum class ExpectedFunction { ACCESS, LEAVE, RELEASE, ANY };

    // état des trains
    enum class State { FREE, TAKEN, WAITING_SAME_D, WAITING_DIFFERENT_D, CLOSED};

    /**
     * @brief SharedSection Constructeur de la classe qui représente la section partagée.
     * Initialisez vos éventuels attributs ici, sémaphores etc.
     */
    SharedSection() : semaphore{0}, mutex{1}, state{State::FREE} {
    }

    /**
     * @brief Request access to the shared section
     * @param Locomotive who asked access
     * @param Direction of the locomotive
     */
    void access(Locomotive &loco, Direction d) override {
        mutex.acquire(); // on acquire le mutex afin de proteger la section critique
        if (state == State::CLOSED) { // si la section est fermée, on release le mutex
            mutex.release();
            semaphore.acquire(); // on demande l'accès au trançon
            mutex.acquire();
        }
        if (&loco == currentLoco || &loco == waitingLoco) { //verification d'erreurs
            errors++;
            mutex.release();
            return;
        }
        bool willBlock = true;
        if (state == State::FREE) { //cas ou section est libre
            state = State::TAKEN;
            currentLoco = &loco;
            currentDirection = d;
            nextFunction = ExpectedFunction::LEAVE;
            willBlock = false;
        } else { //cas ou section n'est pas libre (forcement TAKEN car il y a que 2 loco)
            waitingLoco = &loco;
            waitingDirection = d;
            if (d != currentDirection) { //selection de l'etat en fonction de la direction
                state = State::WAITING_DIFFERENT_D;
            } else {
                state = State::WAITING_SAME_D;
                if (nextFunction == ExpectedFunction::RELEASE) { //si la loco précédente a deja leave la section on peut s'engager
                    willBlock = false;
                    nextFunction = ExpectedFunction::ANY;
                }
            }
        }
        mutex.release();
        if (willBlock) { // si cela bloque, on se met en attente de l'autre train.
            loco.arreter();
            semaphore.acquire();    //attente du release ou leave
            mutex.acquire();
            state = State::TAKEN;
            currentLoco = &loco;
            currentDirection = d;
            waitingLoco = nullptr;
            nextFunction = ExpectedFunction::LEAVE;
            loco.demarrer();
            mutex.release();
        }
    }

    /**
     * @brief Notify the shared section that a Locomotive has left (not freed yed).
     * @param Locomotive who left
     * @param Direction of the locomotive
     */
    void leave(Locomotive &loco, Direction d) override {
        mutex.acquire();
        //verification d'erreurs
        if (waitingLoco != &loco && currentLoco != &loco) {
            errors++;
            mutex.release();
            return;
        }
        if (waitingLoco == &loco && waitingDirection != d) {
            errors++;
            mutex.release();
            return;
        }
        if (currentLoco == &loco && currentDirection != d) {
            errors++;
            mutex.release();
            return;
        }
        if (nextFunction != ExpectedFunction::LEAVE && nextFunction != ExpectedFunction::ANY) {
            errors++; //erreur d'ordre mais on continue le programme
        }
        nextFunction = ExpectedFunction::RELEASE;

        // Cas meme direction
        if (state == State::WAITING_SAME_D) {
            if (currentLoco == &loco && currentDirection == d) {
                semaphore.release();
            }
            nextFunction = ExpectedFunction::ANY;// any car on ne sais pas quelle loco arrivera à un point en premier

            // cas différente direction
        } else if (state == State::WAITING_DIFFERENT_D) {
            // encore plus de verification d'erreurs
            if (&loco == waitingLoco) {
                errors++;
                mutex.release();
                return;
            }
            if (currentLoco != &loco) {
                errors++;
                mutex.release();
                return;
            }
            if (currentDirection != d) {
                errors++;
                mutex.release();
                return;
            }
            //semaphore will be released when the first loco calls release
        }
        mutex.release();
    }

    /**
     * @brief Notify the shared section that it can now be accessed again (freed).
     * @param Locomotive who sent the notification
     */
    void release(Locomotive &loco) override {
        mutex.acquire();

        if (waitingLoco != &loco && currentLoco != &loco) {
            errors++;
            mutex.release();
            return;
        }
        if (nextFunction != ExpectedFunction::RELEASE && nextFunction != ExpectedFunction::ANY) {
            errors++;
        }

        switch (state) {
            case State::WAITING_DIFFERENT_D:
                semaphore.release();
                break;
            case State::WAITING_SAME_D:
                state = State::TAKEN;
                currentLoco = waitingLoco;
                currentDirection = waitingDirection;
                waitingLoco = nullptr;
                nextFunction = ExpectedFunction::ANY;
                break;
            case State::TAKEN:
                if (currentLoco == &loco) {
                    state = State::FREE;
                    nextFunction = ExpectedFunction::ACCESS;
                    currentLoco = nullptr;
                }
                break;
            default:
                break;
        }
        mutex.release();
    }

    /**
     * @brief Stop all locomotives to access this shared section
     */
    void stopAll() override {
        mutex.acquire();
        state = State::CLOSED;
        mutex.release();
    }

    /**
     * @brief Return nbErrors
     * @return nbErrors
     */
    int nbErrors() override {
        return errors;
    }

    //getter for easier testing
    [[nodiscard]] State getState() const {
        return state;
    }

private:
    /*
     * Vous êtes libres d'ajouter des méthodes ou attributs
     * pour implémenter la section partagée.
     */
    int errors = 0;
    // pointeur sur la première loco initialisée. On l'utilise pour savoir quelle thread est quelle loco
    Locomotive *currentLoco = nullptr;

    //pointeur sur la locomotive qui est entrain d'attendre
    Locomotive *waitingLoco = nullptr;
    Direction currentDirection;
    Direction waitingDirection;

    // le semaphore "semaphore" qui va gerer les threads des trains
    PcoSemaphore semaphore;
    // Semaphore "mutex" permettant de gerer les accès aux variables donc au section critique
    PcoSemaphore mutex;
    bool stopped = false;
    ExpectedFunction nextFunction = ExpectedFunction::ACCESS;
    State state = State::FREE;
};


#endif // SHAREDSECTION_H
