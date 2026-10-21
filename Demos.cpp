
#include <iostream>
#include <fstream> // ofstream para escribir en archivo
#include <string>
#include <thread>
#include <vector>
#include "foreach.h"
#include "containers/vector.h"
#include "containers/linkedlist.h"
#include "containers/heap.h"
#include "containers/binarytree.h"
#include "Demos.h"
using namespace std;

void AddOne(GeneralNode<TX> &node) {
    node.value() += 1;
}

template <typename T>
void AddX(GeneralNode<T> &node, T x) {
    node.value() += x;
}

void Square(GeneralNode<TX> &node) {
    node.value() *= node.value();
}

// Predicado generico para FirstThat/call: true si el valor del nodo es
// mayor que 'threshold'. Generico en Node y T para servir tanto a
// Vector<VectorTraits<TX>> (int) como a Vector<VectorTraits<string>>.
template <typename Node, typename T>
bool IsGreaterThan(Node &node, T threshold) {
    return node.getValue() > threshold;
}

template <typename Node>
void PrintNode(Node &node, ostream &os) {
    os << node << " ";
}
const int NThreads = 5;

// Inserta secuencialmente cada pareja (valor, ref) de 'values' en el
// Container, una por una. La insercion concurrente queda aislada en
// DemoRaceCondition(), que es donde se estudia esa problematica.
template <typename Container>
void InsertElements(Container &container,
                     const vector<pair<typename Container::value_type, Ref>> &values) {
    for (const auto &v : values)
        container.push_back(v.first, v.second);
}

template <typename Container>
void TestContainer(Container &container,
                    const vector<pair<typename Container::value_type, Ref>> &values,
                    const string &filename) {
    InsertElements(container, values);

    // Impresion usando write()
    cout << "Container using write(): ";
    container.write(cout);
    cout << endl;

    // Escritura hacia un archivo, en modo append para acumular cada estado
    // (el archivo se deja vacio una vez al inicio, ver DemoVector)
    ofstream of(filename, ios::app);
    container.write(of);
    of << endl;
    of.close();

    // Escritura en pantalla usando cout directamente (operator<<)
    cout << "Container using cout directly: ";
    cout << container << endl;
}

// Prueba los recorridos del Container hacia adelante (begin/end) y hacia
// atras (rbegin/rend) solamente, imprimiendo los elementos en cada sentido.
template <typename Container>
void TestTraversal(Container &container) {
    using Node = typename Container::Node;

    cout << "Forward traversal:  [";
    container.call(PrintNode<Node>, cout);
    cout << "]" << endl;

    cout << "Backward traversal: [";
    container.rcall(PrintNode<Node>, cout);
    cout << "]" << endl;

    // Prueba de call()/FirstThat() con valor de retorno: busca el primer
    // nodo cuyo valor sea mayor que el del primer elemento del container.
    if (!container.empty()) {
        auto threshold = (*container.begin()).getValue();
        Node &found = container.FirstThat(IsGreaterThan<Node, decltype(threshold)>, threshold);
        cout << "FirstThat (primer valor > " << threshold << "): " << found << endl;
    }
}

// Prueba los recorridos únicamente hacia adelante
template <typename Container>
void TestForwardTraversal(Container &container){
    using Node = typename Container::Node;

    cout << "Prueba de función call con std::cout para LinkedList [";
    container.call(PrintNode<Node>, cout);
    cout << "]" << endl;

    cout << "Prueba de uso de iteradores para impresión: [";
    for (auto &&node : container)
        cout << node << " ";
    cout << "]" << endl;

    // Prueba de FirstThat(): primer nodo cuyo valor sea mayor que el primero
    if (container.begin() != container.end()) {
        auto threshold = (*container.begin()).getValue();
        Node &found = container.FirstThat(IsGreaterThan<Node, decltype(threshold)>, threshold);
        cout << "FirstThat (primer valor > " << threshold << "): " << found << endl;
    }
}

void DemoVector() {
    // Dejamos los archivos vacios para que TestContainer acumule (append)
    // el estado del container tras cada paso
    ofstream("vector.txt", ios::trunc).close();

    // Cada elemento es una pareja (valor, ref) que se guarda en un Node;
    // el constructor initializer_list arma el Vector inicial de una vez
    Vector<VectorTraits<TX>> vec({{0, 10}, {1, 11}, {2, 12}, {3, 13}, {4, 14}});
    TestContainer(vec, {{5, 15}, {6, 16}, {7, 17}, {8, 18}, {9, 19}}, "vector.txt");
    TestTraversal(vec);

    ofstream("vector_str.txt", ios::trunc).close();
    Vector<VectorTraits<string>> strVec;
    TestContainer(strVec, {{"Hello", 1}, {"World", 2}}, "vector_str.txt");
    TestTraversal(strVec);
}

// Insertamos muchos elementos (generados en un loop, no a mano)
// concurrentemente y comparamos cuantos deberian haber entrado contra
// cuantos entraron realmente. Si push_back no estuviera sincronizado
// (sin el mutex/lock_guard actual) esto perderia inserciones o crashearia;
// con el mutex protegiendo push_back/resize, deberia dar siempre 0 perdidas.
void DemoRaceCondition() {
    const size_t N = 200000;

    vector<pair<TX, Ref>> values(N);
    for (size_t i = 0; i < N; ++i)
        values[i] = {static_cast<TX>(i), static_cast<Ref>(i)};

    Vector<VectorTraits<TX>> vec;

    // Insercion concurrente: NThreads workers insertando en paralelo sobre
    // el mismo Vector, cada uno con un subconjunto entrelazado (stride)
    size_t n = values.size();
    vector<thread> workers;
    for (int t = 0; t < NThreads; ++t) {
        workers.emplace_back([&vec, &values, n, t](){
            for (size_t i = t; i < n; i += NThreads)
                vec.push_back(values[i].first, values[i].second);
        });
    }
    for (auto &worker : workers)
        worker.join();

    long long expectedSum = 0;
    for (auto &v : values) expectedSum += v.first;

    long long actualSum = 0;
    for (size_t i = 0; i < vec.size(); ++i) actualSum += vec[i].getValue();

    cout << "DemoRaceCondition: se esperaban " << N << " elementos, "
         << "el Vector quedo con " << vec.size() << endl;
    cout << "  suma esperada = " << expectedSum
         << ", suma obtenida = " << actualSum << endl;

    if (vec.size() != N || actualSum != expectedSum)
        cout << "  *** Race condition detectada: se perdieron inserciones (push_back / resize sin sincronizar) ***" << endl;
    else
        cout << "  No se perdio ningun elemento: el mutex de push_back/resize "
             << "sincroniza correctamente las inserciones concurrentes" << endl;
}

template <typename List>
void RunListDemo(const string &filename) {
    ofstream(filename, ios::trunc).close();

    // Pruebas de push_back
    List list;
    TestContainer(list, {{5, 15}, {1, 11}, {8, 18}, {3, 13}}, filename);
    TestForwardTraversal(list);

    auto elements = std::vector<pair<TX, Ref>>({{0, 10}, {1, 11}, {2, 12}, {3, 13}, {4, 14}});
    for (auto &e : elements)
        list.push_back(e.first, e.second);

    // Pruebas de insert
    list.insert(4, 12);
    list.insert(11, 20);
    list.insert(7, 6);
    cout << "Lista luego de hacer 3 inserts " << list << endl;

    // Prueba de ApplyFunction
    list.ApplyFunction(AddOne);
    cout << "Lista tras usar AddOne a sus elementos: " << list << endl;
    list.ApplyFunction(AddX<TX>, TX(10));
    cout << "Lista tras usar AddX(10) a sus elementos: " << list << endl;

    // Prueba de clear
    list.clear();
    cout << "Lista luego de usar clear " << list << endl;
    list.push_back(10, 2);
    cout << "Lista luego de un nuevo push_back " << list << endl;

    // Prueba de copia
    List copia(list);
    cout << "Copia de la lista: " << copia << endl;

    // Prueba de lectura desde archivo
    List new_list;
    ifstream in(filename);
    in >> new_list;
    in.close();
    cout << "Lista leida desde archivo: " << new_list << endl;
    TestForwardTraversal(new_list);
}

void DemoLinkedList() {
    cout << "=== LinkedList simple ===" << endl;
    RunListDemo<AscLinkedList<TX>>("linkedlist.txt");
}

void DemoCircularLinkedList() {
    cout << "=== LinkedList circular ===" << endl;
    using IntCircularLinkedList = AscCircularLinkedList<TX>;
    RunListDemo<IntCircularLinkedList>("circularlinkedlist.txt");
}

void DemoDoubleLinkedList() {
    cout << "=== LinkedList doble ===" << endl;
    using IntDoubleLinkedList = AscDoubleLinkedList<TX>;
    RunListDemo<IntDoubleLinkedList>("doublelinkedlist.txt");
}

void DemoCircularDoubleLinkedList() {
    cout << "=== LinkedList circular doble ===" << endl;
    using IntCircularDoubleLinkedList = AscCircularDoubleLinkedList<TX>;
    RunListDemo<IntCircularDoubleLinkedList>("circulardoublelinkedlist.txt");

    // Prueba extra: recorrido hacia atrás
    IntCircularDoubleLinkedList list({{1, 11}, {2, 12}, {3, 13}});
}

template <typename Heap>
bool CheckPopOrder(const Heap &heap, ostream *os = nullptr) {
    typename Heap::Compare comp;
    Heap copia(heap);
    bool ok = true, first = true;
    typename Heap::value_type prev{};
    while (!copia.empty()) {
        auto node  = copia.pop();
        auto value = node.getValue();
        if (!first && comp(value, prev)) ok = false;
        if (os) *os << node << " ";
        prev  = value;
        first = false;
    }
    return ok;
}


template <typename Heap>
void TestHeapConcurrency() {
    const size_t N = 20000;
    Heap heap;
    vector<thread> workers;
    for (int t = 0; t < NThreads; ++t)
        workers.emplace_back([&heap, t]() {
            for (size_t i = t; i < N; i += NThreads)
                heap.push(static_cast<TX>(i), static_cast<Ref>(i));
        });
    for (auto &w : workers) w.join();

    bool ok = heap.size() == N && CheckPopOrder(heap);
    cout << "Push concurrente: " << heap.size() << "/" << N << " elementos, "
         << (ok ? "heap valido" : "*** heap corrupto ***") << endl;
}

template <typename Heap>
void RunHeapDemo(const string &filename){
    using Node = typename Heap::Node;
    ofstream(filename, ios::trunc).close();

    // Construcción con initializer_list
    Heap heap({{5, 15}, {-3, 13}, {8, 18}, {-1, 11}, {2, 12}});
    cout << "Heap inicial (orden interno por niveles): " << heap << endl;
    cout << "top(): " << heap.top() << endl;

    // push uno por uno (HeapifyUp)
    for (auto &e : vector<pair<TX, Ref>>{{7, 17}, {-6, 16}, {0, 10}, {4, 14}})
        heap.push(e.first, e.second);
    cout << "Heap tras 4 push: " << heap << endl;
    cout << "top(): " << heap.top() << endl;

    // Recorrido: muestran el orden interno
    cout << "Forward traversal:  [";
    heap.call(PrintNode<Node>, cout);
    cout << "]" << endl;
    cout << "Backward traversal: [";
    heap.rcall(PrintNode<Node>, cout);
    cout << "]" << endl;

    // FirstThat: primer nodo con valor > 0
    try {
        Node found = heap.FirstThat(IsGreaterThan<Node, TX>, TX(0));
        cout << "FirstThat (primer valor > 0): " << found << endl;
    } catch (const runtime_error &e) {
        cout << "FirstThat: " << e.what() << endl;
    }

    // pop sobre el heap real
    Node raiz = heap.pop();
    cout << "pop(): " << raiz << " -> heap: " << heap << endl;

    // Aplicación de ApplyFuncion sin una función que altera el orden
    heap.ApplyFunction(AddOne);
    cout << "Heap tras AddOne: " << heap << endl;
    heap.ApplyFunction(AddX<TX>, TX(10));
    cout << "Heap tras AddX(10): " << heap << " -> heap valido? " << (CheckPopOrder(heap) ? "si" : "no") << endl;

    Heap cuadrados(heap);
    cuadrados.ApplyFunction(Square);
    cuadrados.build_heap();
    cout << "Copia tras Square: " << cuadrados << " -> heap valido? "
         << (CheckPopOrder(cuadrados) ? "si" : "no") << endl;

    // Escritura a archivo
    ofstream of(filename, ios::app);
    of << heap << endl;
    of.close();

    // clear + manejo de heap vacío
    heap.clear();
    cout << "Heap tras clear: " << heap << " (empty: " << (heap.empty() ? "si" : "no") << ")" << endl;
    try {
        heap.top();
    } catch (const out_of_range &e) {
        cout << "top() en heap vacio: " << e.what() << endl;
    }

    // Copia
    heap.push(10, 2);
    Heap copia(heap);
    cout << "Copia del heap: " << copia << endl;

    // Lectura desde archivo
    Heap leido;
    ifstream in(filename);
    in >> leido;
    in.close();
    cout << "Heap leido desde archivo: " << leido << " -> heap valido? " << (CheckPopOrder(leido) ? "si" : "no") << endl;

    TestHeapConcurrency<Heap>();
}

void DemoMinHeap(){
    cout << "=== Min Heap ===" << endl;
    using IntMinHeap = MinHeap<TX>;
    RunHeapDemo<IntMinHeap>("intminheap.txt");
}

void DemoMaxHeap(){
    cout << "=== Max Heap ===" << endl;
    using IntMaxHeap = MaxHeap<TX>;
    RunHeapDemo<IntMaxHeap>("intmaxheap.txt");
}

template <typename View>
void PrintTraversal(const string &name, const View &view) {
    using Node = typename View::Node;
    cout << name << ": [";
    view.call(PrintNode<Node>, cout);
    cout << "]  inverso: [";
    view.rcall(PrintNode<Node>, cout);
    cout << "]" << endl;
}

template <typename Tree>
void RunBSTDemo(const string &filename){
    using Node = typename Tree::Node;
    ofstream(filename, ios::trunc).close();

    // Construccion con initializer list
    Tree tree({{5, 15}, {-3, 13}, {8, 18}, {-1, 11}, {2, 12}});
    cout << "BST inicial (persistido en pre-order): " << tree << endl;

    // push uno por uno
    for (auto &e : vector<pair<TX, Ref>>{{7, 17}, {-6, 16}, {0, 10}, {4, 14}})
        tree.push(e.first, e.second);
    cout << "BST tras 4 push: " << tree << " (size: " << tree.size() << ")" << endl;

    // Recorridos: in-order por defecto y las vistas pre/post-order
    PrintTraversal("In-order  ", tree);
    PrintTraversal("Pre-order ", tree.preorder());
    PrintTraversal("Post-order", tree.postorder());

    // FirstThat: depende del recorrido usado
    try {
        Node &found = tree.FirstThat(IsGreaterThan<Node, TX>, TX(0));
        cout << "FirstThat in-order  (primer valor > 0): " << found << endl;
        Node &foundPre = tree.preorder().FirstThat(IsGreaterThan<Node, TX>, TX(0));
        cout << "FirstThat pre-order (primer valor > 0): " << foundPre << endl;
    } catch (const runtime_error &e) {
        cout << "FirstThat: " << e.what() << endl;
    }

    // clear + arbol vacio
    tree.clear();
    cout << "BST tras clear: " << tree << " (empty: " << (tree.empty() ? "si" : "no") << ")" << endl;

    // Copia y movimiento
    tree.push(10, 2);
    Tree copia(tree);
    cout << "Copia del BST: " << copia << endl;
    Tree movido(std::move(copia));
    cout << "Tras mover: movido = " << movido << ", copia = " << copia << " (empty: " << (copia.empty() ? "si" : "no") << ")" << endl;
}

void DemoBST(){
    cout << "=== Binary Search Tree (BST) ===" << endl;
    using IntBST = BST<TX>;
    RunBSTDemo<IntBST>("intbst.txt");
}

void DemoDescBST(){
    cout << "=== Descending Binary Search Tree (DescBST) ===" << endl;
    using IntDescBST = DescBST<TX>;
    RunBSTDemo<IntDescBST>("intdescbst.txt");
}
