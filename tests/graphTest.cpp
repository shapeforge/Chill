#include "stdafx.h"
#include "CppUnitTest.h"

#include <stdlib.h> 
#include <time.h>

#include "Processor.h"
#include "ProcessingGraph.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;


TEST_CLASS(ProcessingGraphTest)
{
public:
  TEST_METHOD(cyclicGraphDetection)
  {
    Chill::ProcessingGraph graph;

    AutoPtr<Chill::Processor> p1 = graph.addProcessor<Chill::Processor>();
    AutoPtr<Chill::Processor> p2 = graph.addProcessor<Chill::Processor>();
    AutoPtr<Chill::Processor> p3 = graph.addProcessor<Chill::Processor>();

    p1->addInput("i", IOType::UNDEF); p1->addOutput("o", IOType::UNDEF);
    p2->addInput("i", IOType::UNDEF); p2->addOutput("o", IOType::UNDEF);
    p3->addInput("i1", IOType::UNDEF); p3->addInput("i2", IOType::UNDEF); p3->addOutput("o", IOType::UNDEF);

    graph.connect(p1, "o", p2, "i");
    graph.connect(p2, "o", p3, "i1");

    Assert::IsTrue(graph.connect(p1, "o", p3, "i2"),
      L"Graph detected as invalid, but it wasn't", LINE_INFO());

    // Create a cycle
    Assert::IsFalse(graph.connect(p3, "o", p1, "i"),
      L"Graph not detected as invalid, but it was", LINE_INFO());
  }

  TEST_METHOD(addPipe)
  {
    Chill::ProcessingGraph g1;
    Chill::ProcessingGraph g2;

    AutoPtr<Chill::Processor> g1_p1 = g1.addProcessor<Chill::Processor>();
    AutoPtr<Chill::Processor> g1_p2 = g1.addProcessor<Chill::Processor>();
    AutoPtr<Chill::Processor> g2_p1 = g2.addProcessor<Chill::Processor>();
    
    AutoPtr<Chill::Processor> null = AutoPtr<Chill::Processor>(NULL);

    g1_p1->addInput("io", IOType::UNDEF);  g1_p1->addOutput("io", IOType::UNDEF);
    g1_p2->addInput("io", IOType::UNDEF);
    g2_p1->addInput("io", IOType::UNDEF);

    g1.connect(g1_p1, "io", null, "null_io");


    // valid output
    Assert::IsFalse(g1_p1->output("io").isNull(),
      L"Check if output is registered", LINE_INFO());

    // invalid output
    Assert::IsTrue(g1_p1->output("not_io").isNull(),
      L"Check if output is registered", LINE_INFO());

    // valid input
    Assert::IsFalse(g1_p2->input("io").isNull(),
      L"Check if input is registered", LINE_INFO());

    // invalid input
    Assert::IsTrue(g1_p2->input("not_io").isNull(),
      L"Check if input is registered", LINE_INFO());

    // invalid output
    Assert::IsFalse(g1.connect(g1_p1, "not_io", g1_p2, "io"),
      L"Pipe with a non existing output", LINE_INFO());

    // invalid input
    Assert::IsFalse(g1.connect(g1_p1, "io", g1_p2, "not_io"),
      L"Pipe with a non existing input", LINE_INFO());

    // valid
    Assert::IsTrue(g1.connect(g1_p1, "io", g1_p2, "io"),
      L"Pipe between two valid processors", LINE_INFO());

    // same processor
    Assert::IsFalse(g1.connect(g1_p1, "io", g1_p1, "io"),
      L"Pipe between two sockets from same processor", LINE_INFO());

    // not same graph
    Assert::IsFalse(g1.connect(g1_p1, "io", g2_p1, "io"),
      L"Pipe between two processors in different graph", LINE_INFO());
  }

  TEST_METHOD(removePipe)
  {

  }

  TEST_METHOD(addProcessor)
  {
    AutoPtr<Chill::ProcessingGraph> g1 = AutoPtr<Chill::ProcessingGraph>(new Chill::ProcessingGraph());
    AutoPtr<Chill::Processor> p1 = g1->addProcessor<Chill::Processor>();

    Assert::IsNotNull(p1.raw());
    Assert::IsTrue(g1.raw() == p1->owner());
  }

  TEST_METHOD(removeProcessor)
  {
    AutoPtr<Chill::ProcessingGraph> g1 = AutoPtr<Chill::ProcessingGraph>(new Chill::ProcessingGraph);
    AutoPtr<Chill::Processor> p1 = g1->addProcessor<Chill::Processor>();
    AutoPtr<Chill::Processor> p2 = g1->addProcessor<Chill::Processor>();
    AutoPtr<Chill::Processor> p3 = g1->addProcessor<Chill::Processor>();
    AutoPtr<Chill::Processor> p4 = g1->addProcessor<Chill::Processor>();

    p1->addOutput("io", IOType::UNDEF);
    p2->addInput("io", IOType::UNDEF); p2->addOutput("io", IOType::UNDEF);
    p3->addInput("io", IOType::UNDEF); p3->addOutput("io", IOType::UNDEF);
    p4->addInput("io", IOType::UNDEF);

    g1->connect(p1, "io", p2, "io");
    g1->connect(p2, "io", p3, "io");
    g1->connect(p3, "io", p4, "io");
    g1->connect(p1, "io", p4, "io");

    g1->remove(p2);

    Assert::AreEqual((size_t)3, g1->processors().size(),
      L"Incorrect size", LINE_INFO());

    /*Assert::AreEqual((size_t)2, g1->m_pipes.size(),
      L"Incorrect size", LINE_INFO());*/
  }

  TEST_METHOD(copySubset)
  {
    Assert::Fail(L"Test not written");
  }

  TEST_METHOD(collapseSubset)
  {
    // config
    srand((uint)time(NULL));

    int nb_processors = 10 + rand() % 20;
    int nb_pipes = 5 * nb_processors;
    int nb_ios = 100;

    // vars
    int nb_pipes_created = 0;

    AutoPtr<Chill::ProcessingGraph> g1 = AutoPtr<Chill::ProcessingGraph>(new Chill::ProcessingGraph);

    std::vector<AutoPtr<Chill::Processor>> processors;
    std::vector<AutoPtr<Chill::Processor>> subset;


    ForRange(i, 0, nb_processors) {
      AutoPtr<Chill::Processor> proc = g1->addProcessor<Chill::Processor>();

      ForRange(j, 0, nb_ios) {
        proc->addInput("io" + j, IOType::UNDEF);
        proc->addOutput("io" + j, IOType::UNDEF);
      }

      processors.push_back(proc);
      if (i % 4) {
        subset.push_back(proc);
      }
    }

    ForRange(i, 0, nb_pipes) {
      int a = rand() % nb_processors;
      int b = rand() % nb_processors;

      if (a > b) {
        std::swap(a, b);
      }

      AutoPtr<Chill::Processor> from = processors[a];
      AutoPtr<Chill::Processor>   to = processors[b];
      std::string from_io       = "io" + (rand() % nb_ios);
      std::string to_io         = "io" + (rand() % nb_ios);
      
      if (g1->connect(from, from_io, to, to_io)) {
        nb_pipes_created++;
      }
    }

    AutoPtr<Chill::ProcessingGraph> g2 = g1->collapseSubset(subset);

    // Check graph size
    Assert::AreEqual(subset.size(), g2->processors().size(),
      L"Incorrect size, not all selected processors are in the new graph", LINE_INFO());

    Assert::AreEqual(1 + nb_processors - (subset.size()-1), g1->processors().size(),
      L"Incorrect size, maybe the new ProcessingGraph is not registered as a processor", LINE_INFO());

    /*size_t nb_pipes_after = 0;
    for (Chill::Pipe_Ptr pipe : pipes) {
      auto from_iter = find(subset.begin(), subset.end(), pipe->m_from);
      auto to_iter   = find(subset.begin(), subset.end(), pipe->m_to);

      if (from_iter != subset.end() && to_iter == subset.end()) { 
        nb_pipes_after++;
      }
      if (from_iter == subset.end() && to_iter != subset.end()) {
        nb_pipes_after++;
      }
    }
    nb_pipes_after += pipes.size();

    Assert::AreEqual(pipes.size(), g1->m_pipes.size() + g2->m_pipes.size(),
      L"Incorrect size, pipes missing or duplicate", LINE_INFO());
    

    /*
    AutoPtr<Chill::ProcessingGraph> g1 = AutoPtr<Chill::ProcessingGraph>(new Chill::ProcessingGraph);
    AutoPtr<Chill::Processor> p1 = g1->addProcessor<Chill::Processor>();
    AutoPtr<Chill::Processor> p2 = g1->addProcessor<Chill::Processor>();
    AutoPtr<Chill::Processor> p3 = g1->addProcessor<Chill::Processor>();
    AutoPtr<Chill::Processor> p4 = g1->addProcessor<Chill::Processor>();

    p1->addOutput("io");
    p2->addInput("io"); p2->addOutput("io");
    p3->addInput("io"); p3->addOutput("io");
    p4->addInput("io");

    g1->addPipe(p1, "io", p2, "io");
    g1->addPipe(p2, "io", p3, "io");
    g1->addPipe(p3, "io", p4, "io");
    g1->addPipe(p1, "io", p4, "io");

    std::vector<AutoPtr<Chill::Processor>> subset;
    subset.emplace_back(p2);
    AutoPtr<Chill::ProcessingGraph> g2 = g1->collapseSubset(subset);

    Assert::AreEqual((size_t)4, g1->m_processors.size(),
      L"Incorrect size", LINE_INFO());

    Assert::AreEqual((size_t)2, g1->m_pipes.size(),
      L"Incorrect size", LINE_INFO());
    */
  }
};