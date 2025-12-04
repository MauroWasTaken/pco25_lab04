//  /$$$$$$$   /$$$$$$   /$$$$$$         /$$$$$$   /$$$$$$   /$$$$$$  /$$$$$$$ 
// | $$__  $$ /$$__  $$ /$$__  $$       /$$__  $$ /$$$_  $$ /$$__  $$| $$____/ 
// | $$  \ $$| $$  \__/| $$  \ $$      |__/  \ $$| $$$$\ $$|__/  \ $$| $$      
// | $$$$$$$/| $$      | $$  | $$        /$$$$$$/| $$ $$ $$  /$$$$$$/| $$$$$$$ 
// | $$____/ | $$      | $$  | $$       /$$____/ | $$\ $$$$ /$$____/ |_____  $$
// | $$      | $$    $$| $$  | $$      | $$      | $$ \ $$$| $$       /$$  \ $$
// | $$      |  $$$$$$/|  $$$$$$/      | $$$$$$$$|  $$$$$$/| $$$$$$$$|  $$$$$$/
// |__/       \______/  \______/       |________/ \______/ |________/ \______/ 


#include "locomotivebehavior.h"
#include "ctrain_handler.h"

void LocomotiveBehavior::run() {
    //Initialisation de la locomotive
    loco.allumerPhares();
    loco.demarrer();
    loco.afficherMessage("Ready!");

    int index = 1;              //index pour savoir ou la loc se dirige
    int incrementor = 1;        //variable avec laquelle on va affecter index (positif pour d1 negatif pour d2)
    SharedSectionInterface::Direction dir = SharedSectionInterface::Direction::D1; // D1 pour sens de la montre
    while (true) {
        if (&locoA == &loco) {
            // red
            attendre_contact(ROUTERED[index]);
            if (ROUTERED[index] == SECTIONCRITIQUERED[0]) {
                sharedSection->access(loco, dir);
            }
            if (ROUTERED[index] == LEAVESECTIOND1) {
                sharedSection->leave(loco, dir);
            }
            if (ROUTERED[index] == SECTIONCRITIQUERED[1]) {
                sharedSection->release(loco);
            }

            if (ROUTERED[index] == AGUILLAGETRIGGER) {
                diriger_aiguillage(AGUILLAGE, DEVIE, 0);
            }
            index = (index + incrementor) % ROUTERED.size();
        } else {
            // blue
            attendre_contact(ROUTEBLUE[index]);
            if (ROUTEBLUE[index] == SECTIONCRITIQUEBLUE[0]) {
                if (dir == SharedSectionInterface::Direction::D1) {
                    sharedSection->access(loco, dir);
                } else {
                    sharedSection->release(loco);
                }
            }
            if (ROUTEBLUE[index] == SECTIONCRITIQUEBLUE[1]) {
                if (dir == SharedSectionInterface::Direction::D1) {
                    sharedSection->release(loco);
                } else {
                    sharedSection->access(loco, dir);
                }
            }
            if (dir == SharedSectionInterface::Direction::D1) {
                if (ROUTEBLUE[index] == LEAVESECTIOND1) {
                    sharedSection->leave(loco, dir);
                }
            } else {
                if (ROUTEBLUE[index] == LEAVESECTIOND2) {
                    sharedSection->leave(loco, dir);
                }
            }
            //changement de sens
            if (ROUTEBLUE[index] == ROUTEBLUE.back()) {
                loco.inverserSens();
                dir = SharedSectionInterface::Direction::D2;
                incrementor = -1;
            }
            if (ROUTEBLUE[index] == ROUTEBLUE[0]) {
                loco.inverserSens();
                dir = SharedSectionInterface::Direction::D1;
                incrementor = 1;
            }
            //direction d'aiguillage
            if (ROUTERED[index] == AGUILLAGETRIGGER) {
                diriger_aiguillage(AGUILLAGE, TOUT_DROIT, 0);
            }
            index += incrementor;
        }
    }
}


void LocomotiveBehavior::printStartMessage() {
    qDebug() << "[START] Thread de la loco" << loco.numero() << "lancé";
    loco.afficherMessage("Je suis lancée !");
}

void LocomotiveBehavior::printCompletionMessage() {
    qDebug() << "[STOP] Thread de la loco" << loco.numero() << "a terminé correctement";
    loco.afficherMessage("J'ai terminé");
}
