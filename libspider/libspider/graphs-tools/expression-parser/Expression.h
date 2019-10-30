/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2019) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2015)
 * Yaset Oliva <yaset.oliva@insa-rennes.fr> (2013 - 2014)
 *
 * Spider is a dataflow based runtime used to execute dynamic PiSDF
 * applications. The Preesm tool may be used to design PiSDF applications.
 *
 * This software is governed by the CeCILL  license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */
#ifndef SPIDER2_EXPRESSION_H
#define SPIDER2_EXPRESSION_H

/* === Includes === */

#include <cstdint>
#include <string>
#include <containers/Array.h>
#include <graphs-tools/expression-parser/RPNConverter.h>

/* === Structure definition(s) === */

struct ExpressionTreeNode {
    ExpressionTreeNode *left = nullptr;
    ExpressionTreeNode *right = nullptr;
    ExpressionTreeNode *parent = nullptr;
    RPNElement elt;
    union {
        RPNOperatorType operatorType;
        double value = 0;
        PiSDFParam *param;
    } arg;

    ExpressionTreeNode(const ExpressionTreeNode &) = default;

    ExpressionTreeNode(ExpressionTreeNode *parent, RPNElement elt) : parent{parent},
                                                                     elt{std::move(elt)} {
        right = nullptr;
        left = nullptr;
    }
};

/* === Class definition === */

class Expression {
public:

    explicit Expression(std::string expression, const PiSDFGraph *graph = Spider::pisdfGraph());

    explicit Expression(std::int64_t value);

    Expression() = default;

    Expression(const Expression &other) = default;

    inline Expression(Expression &&other) noexcept : Expression() {
        swap(*this, other);
    }

    ~Expression() = default;

    /* === Operator(s) === */

    inline friend void swap(Expression &first, Expression &second) noexcept {
        /* == Enable ADL == */
        using std::swap;

        /* == Swap members of both objects == */
        swap(first.expressionTree_, second.expressionTree_);
        swap(first.value_, second.value_);
        swap(first.static_, second.static_);
    }

    inline Expression &operator=(Expression temp) {
        swap(*this, temp);
        return *this;
    }

    /* === Methods === */

    /**
     * @brief Evaluate the expression and return the value and cast result in int64_.
     * @return Evaluated value of the expression.
     */
    inline std::int64_t evaluate() const;

    /**
     * @brief Evaluate the expression and return the value.
     * @return Evaluated value of the expression.
     */
    inline double evaluateDBL() const;

    /**
     * @brief Print the ExpressionTree (debug only).
     */
    void printExpressionTree();

    /**
     * @brief Get the infix expression string
     * @return Clean infix expression string
     */
    inline std::string toString() const;

    /**
     * @brief Get the expression postfix string.
     * @return postfix expression string.
     */
    inline std::string postfixString() const;

    /* === Getters === */

    /**
     * @brief Get the last evaluated value (faster than evaluated on static expressions)
     * @return last evaluated value (default value, i.e no evaluation done, is 0)
     */
    inline std::int64_t value() const;

    /**
     * @brief Get the static property of the expression.
     * @return true if the expression is static, false else.
     */
    inline bool isStatic() const;

private:
    Spider::vector<ExpressionTreeNode> expressionTree_;
    double value_ = 0.;
    bool static_ = true;

    /* === Private method(s) === */

    /**
     * @brief Build and reduce the expression tree parser.
     * @param expressionStack Stack of the postfix expression elements.
     */
    void buildExpressionTree();

    /**
     * @brief  Insert current node in the expression tree and reduce it if possible.
     * @param node    Node to insert.
     * @param elt     Associated element to be set in the node.
     * @param nodeIx  Index of the node in the global node array.
     * @return next node
     */
    ExpressionTreeNode *insertExpressionTreeNode(ExpressionTreeNode *node);

    /**
     * @brief Evaluate the value of a node in the ExpressionTree.
     * @param node  Node to evaluate.
     * @return value of the evaluated node.
     * @remark This is a recursive method.
     */
    double evaluateNode(const ExpressionTreeNode *node) const;

    static void printExpressionTreeNode(ExpressionTreeNode *node, std::int32_t depth);
};

/* === Inline methods === */

std::int64_t Expression::value() const {
    return static_cast<std::int64_t>(value_);
}

std::string Expression::toString() const {
//    return rpnConverter_.infixString();
    return "not implemented yet";
}

std::string Expression::postfixString() const {
//    return rpnConverter_.postfixString();
    return "not implemented yet";
}

std::int64_t Expression::evaluate() const {
    if (static_) {
        return static_cast<std::int64_t>(value_);
    }
    return static_cast<std::int64_t>(evaluateNode(&expressionTree_[0]));
}

double Expression::evaluateDBL() const {
    if (static_) {
        return value_;
    }
    return evaluateNode(&expressionTree_[0]);
}

bool Expression::isStatic() const {
    return static_;
}

#endif //SPIDER2_EXPRESSION_H
