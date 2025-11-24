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

void LocomotiveBehavior::run()
{
    //Initialisation de la locomotive
    loco.allumerPhares();
    loco.demarrer();
    loco.afficherMessage("Ready!");

    /* A vous de jouer ! */

    // Vous pouvez appeler les méthodes de la section partagée comme ceci :

    while(true) {
        // On attend qu'une locomotive arrive sur le contact 1.
        // Pertinent de faire ça dans les deux threads? Pas sûr...


        if (loco.numero() == 7) {
            attendre_contact(1);
            SharedSectionInterface::Direction direction = loco.vitesse() > 0 ? SharedSectionInterface::Direction::D1 : SharedSectionInterface::Direction::D2;
            sharedSection->access(loco,direction);
            attendre_contact(31);
            sharedSection->leave(loco,direction);
            sharedSection->release(loco);
            diriger_aiguillage(22, DEVIE, 0);
        }else {
            attendre_contact(1);
            SharedSectionInterface::Direction direction = loco.vitesse() > 0 ? SharedSectionInterface::Direction::D1 : SharedSectionInterface::Direction::D2;
            sharedSection->access(loco,direction);
            attendre_contact(31);
            sharedSection->leave(loco,direction);
            sharedSection->release(loco);
            diriger_aiguillage(22, TOUT_DROIT, 0);

        }
    }
}


void LocomotiveBehavior::printStartMessage()
{
    qDebug() << "[START] Thread de la loco" << loco.numero() << "lancé";
    loco.afficherMessage("Je suis lancée !");
}

void LocomotiveBehavior::printCompletionMessage()
{
    qDebug() << "[STOP] Thread de la loco" << loco.numero() << "a terminé correctement";
    loco.afficherMessage("J'ai terminé");
}
