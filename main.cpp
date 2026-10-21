#include <iostream>
#include "Demos.h"
#include "containers/vector.h"

using namespace std;

// Compilar asi: make
// Ejecutar asi: ./main
int main() {
    DemoVector();
    cout << endl;
    DemoRaceCondition();
    cout << endl;
    DemoLinkedList();
    cout << endl;
    DemoCircularLinkedList();
    cout << endl;
    DemoDoubleLinkedList();
    cout << endl;
    DemoCircularDoubleLinkedList();
    cout << endl;
    DemoMinHeap();
    cout << endl;
    DemoMaxHeap();
    cout << endl;
    DemoBST();
    cout << endl;
    DemoDescBST();
    cout << endl;
    return 0;
}