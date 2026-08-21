#!/usr/bin/env python3
"""
Самостоятельный запуск sc-агентов TrustModule на живом sc-server --
без C++ (test_*.cpp) и без браузерной JS-консоли.

Требует py-sc-client (пакет `sc_client`, входит в venv sc-web):
  <ostis-web-platform>/interface/sc-web/.venv/bin/python3 run_agent.py [имя ...]

Платформа должна быть запущена (`./start.sh` в ostis-web-platform) --
sc-server слушает ws://localhost:8090/ws_json.

Без аргументов запускает демо-сценарии для всех 10 агентов по очереди.
С аргументами -- только перечисленные (см. DEMOS в конце файла для
списка имён, например: `run_agent.py aggregate_trust resolve_conflict`).

Каждый демо-сценарий:
  1) собирает минимальную sc-конструкцию "с нуля" (те же иллюстративные
     числа, что и в соответствующем tests/test_agent_*.cpp -- не трогает
     ни один именованный узел реальной БЗ, поэтому безопасно запускать
     произвольное число раз подряд);
  2) инициирует действие агента (`run_action`) и ждёт завершения;
  3) сверяет результат с ожидаемым числом и печатает OK/MISMATCH;
  4) стирает всё, что создал сам demo-код, и структуру результата
     (erase_elements по явному списку -- см. предупреждение в
     agents_vault про "никогда не стирать по широкому паттерну"). Сам
     узел действия НЕ стирается -- см. предупреждение в docstring
     `run_action`: это надёжно роняет sc-server. Он остаётся в БЗ как
     маленький мусорный узел (несколько sc-элементов на прогон).

Как проверить агента, для которого здесь нет демо (например, свой): смотрите
docstring `run_action` ниже и комментарий-контракт аргументов в
src/agent/agent_<имя>.hpp -- там для каждого агента прямо расписано, что
означает каждый rrel_N (порядковый) аргумент действия.
"""
from __future__ import annotations

import math
import sys
import time

from sc_client import client
from sc_client.constants import sc_types
from sc_client.models import ScConstruction, ScLinkContent, ScLinkContentType, ScTemplate

SC_SERVER_URL = "ws://localhost:8090/ws_json"


# --------------------------------------------------------------------------
# Низкоуровневые помощники (аналог common/float_link_io.hpp,
# common/relation_io.hpp из C++ и LinkByRelation/MakeFloatLink из тестов).
# --------------------------------------------------------------------------


_keynode_cache: dict = {}


def resolve(idtf: str):
    """
    Резолвит существующий кейнод по системному идентификатору, с
    кэшированием в рамках процесса -- без кэша каждый run_action заново
    резолвит rrel_1..rrel_N и все состояния завершения действия на
    каждый вызов, что на этом сборе sc-machine/sc-server (см. README
    предупреждение выше) провоцирует под частым потоком запросов
    падение сервера (uncaught utils::ExceptionInvalidParams, процесс
    sc-server завершается) -- воспроизводится штатно, без правки кода
    агентов, просто плотной последовательностью запросов.
    """
    if idtf not in _keynode_cache:
        _keynode_cache[idtf] = client.resolve_keynodes({"idtf": idtf, "type": None})[0]
    return _keynode_cache[idtf]


def new_node(created: list) -> "ScAddr":
    c = ScConstruction()
    c.generate_node(sc_types.NODE_CONST, "n")
    addr = client.generate_elements(c)[c.get_index("n")]
    created.append(addr)
    return addr


def make_float_link(created: list, value: float) -> "ScAddr":
    c = ScConstruction()
    c.generate_link(sc_types.LINK_CONST, ScLinkContent(str(value), ScLinkContentType.STRING), "link")
    addr = client.generate_elements(c)[c.get_index("link")]
    created.append(addr)
    return addr


def read_float_link(addr) -> float:
    return float(client.get_link_content(addr)[0].data)


def link_by_relation(created: list, src, trg, relation_addr) -> None:
    """src -[ConstCommonArc]-> trg, классифицированная нероле-вым отношением (nrel_*)."""
    c = ScConstruction()
    c.generate_connector(sc_types.EDGE_D_COMMON_CONST, src, trg, "arc")
    c.generate_connector(sc_types.EDGE_ACCESS_CONST_POS_PERM, relation_addr, "arc")
    client.generate_elements(c)


def plain_arc(src, trg) -> None:
    """src -[ConstPermPosArc]-> trg, без классификации (членство в группе)."""
    c = ScConstruction()
    c.generate_connector(sc_types.EDGE_ACCESS_CONST_POS_PERM, src, trg)
    client.generate_elements(c)


def tag_class(node_addr, class_idtf: str) -> None:
    c = ScConstruction()
    c.generate_connector(sc_types.EDGE_ACCESS_CONST_POS_PERM, resolve(class_idtf), node_addr)
    client.generate_elements(c)


def has_child_of_class(parent_addr, class_addr) -> bool:
    tpl = ScTemplate()
    tpl.triple(parent_addr, sc_types.EDGE_ACCESS_VAR_POS_PERM, sc_types.NODE_VAR)
    for result in client.search_by_template(tpl):
        child = result[2]
        tpl2 = ScTemplate()
        tpl2.triple(class_addr, sc_types.EDGE_ACCESS_VAR_POS_PERM, child)
        if client.search_by_template(tpl2):
            return True
    return False


# --------------------------------------------------------------------------
# Общий запуск действия -- аналог ScAction::InitiateAndWait из C++.
# --------------------------------------------------------------------------


def run_action(action_class_idtf: str, args: list, created: list, timeout_sec: float = 5.0, poll_interval: float = 0.2):
    """
    Инициирует sc-действие класса `action_class_idtf` с аргументами `args`
    (агент читает их по порядку через rrel_1, rrel_2, ... -- см.
    ScAction::GetArguments<N>() в sc_action.cpp), ждёт завершения и
    возвращает (status, result_members), где status -- один из
    "successfully" / "unsuccessfully" / "with_error" / "timeout", а
    result_members -- список ScAddr из результирующей структуры
    (то, что C++-агент передал в action.FormResult(...)).

    ВАЖНО (найдено эмпирически при подготовке этого скрипта): стирание
    самого узла действия (`action_addr`) после того, как он уже
    завершился (`action_finished_successfully`/... дуга проведена),
    надёжно роняет sc-server -- необработанное исключение
    `utils::ExceptionInvalidParams` в C++, процесс sc-server падает
    целиком (`libc++abi: terminating due to uncaught exception`).
    Воспроизводится штатной последовательностью запросов клиента, без
    какого-либо изменения кода агентов -- судя по всему, ошибка в самом
    sc-machine (что-то в его внутренней бухгалтерии ожидания завершения
    действия не переживает стирание уже завершённого действия другим
    клиентом). Поэтому узел действия НЕ добавляется в `created` и
    остаётся в БЗ как маленький мусорный узел (несколько sc-элементов на
    вызов, не критично для разового прогона демо) -- в `created`
    попадает только структура результата и то, что явно создал вызывающий
    демо-код.
    """
    action_class_addr = resolve(action_class_idtf)
    action_initiated_addr = resolve("action_initiated")
    finished_states = {
        "successfully": resolve("action_finished_successfully"),
        "unsuccessfully": resolve("action_finished_unsuccessfully"),
        "with_error": resolve("action_finished_with_error"),
    }
    nrel_result_addr = resolve("nrel_result")

    c = ScConstruction()
    c.generate_node(sc_types.NODE_CONST, "action")
    c.generate_connector(sc_types.EDGE_ACCESS_CONST_POS_PERM, action_class_addr, "action")
    for i, arg_addr in enumerate(args, start=1):
        rrel_addr = resolve(f"rrel_{i}")
        c.generate_connector(sc_types.EDGE_ACCESS_CONST_POS_PERM, "action", arg_addr, f"arg_arc_{i}")
        c.generate_connector(sc_types.EDGE_ACCESS_CONST_POS_PERM, rrel_addr, f"arg_arc_{i}")
    action_addr = client.generate_elements(c)[c.get_index("action")]
    # action_addr сознательно не попадает в created -- см. предупреждение выше.

    init_c = ScConstruction()
    init_c.generate_connector(sc_types.EDGE_ACCESS_CONST_POS_PERM, action_initiated_addr, action_addr)
    client.generate_elements(init_c)

    status = "timeout"
    deadline = time.monotonic() + timeout_sec
    while time.monotonic() < deadline:
        for name, addr in finished_states.items():
            tpl = ScTemplate()
            tpl.triple(addr, sc_types.EDGE_ACCESS_VAR_POS_PERM, action_addr)
            if client.search_by_template(tpl):
                status = name
                break
        if status != "timeout":
            break
        time.sleep(poll_interval)

    result_members: list = []
    if status != "timeout":
        tpl = ScTemplate()
        tpl.quintuple(
            action_addr,
            sc_types.EDGE_D_COMMON_VAR,
            sc_types.NODE_VAR_STRUCT,
            sc_types.EDGE_ACCESS_VAR_POS_PERM,
            nrel_result_addr,
        )
        results = client.search_by_template(tpl)
        if results:
            struct_addr = results[0][2]
            created.append(struct_addr)
            tpl2 = ScTemplate()
            tpl2.triple(struct_addr, sc_types.EDGE_ACCESS_VAR_POS_PERM, sc_types.VAR)
            result_members = [m[2] for m in client.search_by_template(tpl2)]

    return status, result_members


def first_link_value(result_members) -> float | None:
    types = client.get_elements_types(*result_members) if result_members else []
    for addr, t in zip(result_members, types):
        if t.is_link():
            return read_float_link(addr)
    return None


def report(name: str, status: str, actual, expected, tol: float = 1e-3) -> None:
    ok = status == "successfully" and actual is not None and math.isclose(actual, expected, abs_tol=tol)
    tag = "OK" if ok else "MISMATCH"
    print(f"[{tag}] {name}: status={status} actual={actual} expected={expected}")


# --------------------------------------------------------------------------
# Демо-сценарии -- по одному на агента, числа воспроизводят
# tests/test_agent_*.cpp / docstring соответствующего .hpp.
# --------------------------------------------------------------------------


def demo_trust_ema(action_idtf: str, alpha_name: str) -> None:
    created: list = []
    try:
        trust_level = new_node(created)
        outcome = make_float_link(created, 1.0)
        status, result = run_action(action_idtf, [trust_level, outcome], created)
        # (1 - alpha) * 0 + alpha * 1.0, alpha = 0.10 во всех четырёх компонентах.
        report(f"{action_idtf} ({alpha_name}=0.10, нейтральный старт)", status, first_link_value(result), 0.10)
    finally:
        if created:
            client.erase_elements(*created)


def demo_aggregate_trust() -> None:
    created: list = []
    try:
        def preset_component(value: float):
            node = new_node(created)
            link = make_float_link(created, value)
            plain_arc(node, link)
            return node

        rel = preset_component(0.8)
        acc = preset_component(0.6)
        sec = preset_component(0.9)
        tr = preset_component(0.7)
        trust_level = new_node(created)

        status, result = run_action("action_aggregate_trust", [rel, acc, sec, tr, trust_level], created)
        # 0.40*0.8 + 0.25*0.6 + 0.15*0.9 + 0.20*0.7 (lambda из agent_aggregate_trust.cpp)
        report("action_aggregate_trust", status, first_link_value(result), 0.745)
    finally:
        if created:
            client.erase_elements(*created)


def demo_compute_pi_metric() -> None:
    created: list = []
    try:
        group = new_node(created)
        nrel_has_provenance = resolve("nrel_has_provenance")
        for has_provenance in (True, True, False):
            element = new_node(created)
            plain_arc(group, element)
            if has_provenance:
                record = new_node(created)
                link_by_relation(created, element, record, nrel_has_provenance)
        metric_level = new_node(created)

        status, result = run_action("action_compute_pi_metric", [group, metric_level], created)
        report("action_compute_pi_metric (2 из 3 с провенансом)", status, first_link_value(result), 2 / 3)
    finally:
        if created:
            client.erase_elements(*created)


def demo_validate_cardinality_constraint() -> None:
    created: list = []
    try:
        relation = new_node(created)
        target = new_node(created)
        for _ in range(2):
            link_by_relation(created, target, new_node(created), relation)
        min_count = make_float_link(created, 1.0)
        max_count = make_float_link(created, 3.0)
        result_level = new_node(created)

        status, result = run_action(
            "action_validate_cardinality_constraint",
            [
                target,
                relation,
                resolve("concept_cardinality_check_outgoing_non_role"),
                min_count,
                max_count,
                resolve("concept_cardinality_constraint_violation"),
                result_level,
            ],
            created,
        )
        report("action_validate_cardinality_constraint (2 связи в [1,3])", status, first_link_value(result), 1.0)
    finally:
        if created:
            client.erase_elements(*created)


def demo_detect_anomalous_request_rate() -> None:
    created: list = []
    try:
        # Тот самый "27x" из иллюстративного сценария в .hpp-комментарии агента.
        rate = make_float_link(created, 27.0)
        baseline = make_float_link(created, 1.0)
        k_sigma = make_float_link(created, 5.0)
        result_level = new_node(created)

        status, result = run_action(
            "action_detect_anomalous_request_rate", [rate, baseline, k_sigma, result_level], created
        )
        report("action_detect_anomalous_request_rate (27x baseline > 5-sigma)", status, first_link_value(result), 1.0)
        if status == "successfully":
            incident_present = has_child_of_class(result_level, resolve("concept_security_incident"))
            print(f"       concept_security_incident создан под result_level: {incident_present}")
    finally:
        if created:
            client.erase_elements(*created)


def demo_resolve_by_trust_weighted() -> None:
    created: list = []
    try:
        def result_with_trust(value: float):
            result_node = new_node(created)
            system = new_node(created)
            link_by_relation(created, result_node, system, resolve("nrel_produced_by"))
            trust_level = new_node(created)
            link_by_relation(created, system, trust_level, resolve("nrel_has_trust_level"))
            link = make_float_link(created, value)
            plain_arc(trust_level, link)
            return result_node

        low = result_with_trust(0.3)
        best = result_with_trust(0.9)
        group = new_node(created)
        plain_arc(group, low)
        plain_arc(group, best)
        conflict = new_node(created)

        status, result = run_action("action_resolve_by_trust_weighted", [group, conflict], created)
        winner_matches = status == "successfully" and best in result
        tag = "OK" if winner_matches else "MISMATCH"
        print(f"[{tag}] action_resolve_by_trust_weighted: status={status} победитель -- система с доверием 0.9: {winner_matches}")
    finally:
        if created:
            client.erase_elements(*created)


def demo_dispatch_ready_subtask() -> None:
    created: list = []
    try:
        subtask = new_node(created)
        system = new_node(created)
        link_by_relation(created, subtask, system, resolve("nrel_assigned_to"))
        link_by_relation(created, subtask, resolve("concept_subtask_status_proposed"), resolve("nrel_subtask_status"))
        # Без зависимостей -- условие "все зависимости завершены" истинно вакуумно.
        result_level = new_node(created)

        status, result = run_action("action_dispatch_ready_subtask", [subtask, result_level], created)
        report("action_dispatch_ready_subtask (назначена, proposed, без зависимостей)", status, first_link_value(result), 1.0)
    finally:
        if created:
            client.erase_elements(*created)


DEMOS = {
    "update_trust_reliability": lambda: demo_trust_ema("action_update_trust_reliability", "alpha"),
    "update_trust_accuracy": lambda: demo_trust_ema("action_update_trust_accuracy", "beta"),
    "update_trust_security": lambda: demo_trust_ema("action_update_trust_security", "gamma"),
    "update_trust_transparency": lambda: demo_trust_ema("action_update_trust_transparency", "delta"),
    "aggregate_trust": demo_aggregate_trust,
    "compute_pi_metric": demo_compute_pi_metric,
    "validate_cardinality_constraint": demo_validate_cardinality_constraint,
    "detect_anomalous_request_rate": demo_detect_anomalous_request_rate,
    "resolve_by_trust_weighted": demo_resolve_by_trust_weighted,
    "dispatch_ready_subtask": demo_dispatch_ready_subtask,
}


def main() -> None:
    names = sys.argv[1:] or list(DEMOS.keys())
    unknown = [n for n in names if n not in DEMOS]
    if unknown:
        print(f"Неизвестные демо: {unknown}. Доступные: {list(DEMOS.keys())}")
        sys.exit(1)

    client.connect(SC_SERVER_URL)
    try:
        for name in names:
            DEMOS[name]()
            time.sleep(0.3)
    finally:
        client.disconnect()


if __name__ == "__main__":
    main()
