#!/usr/bin/env python3
"""Триггер одного из sc-агентов обновления доверия на работающем sc-server.

Агенты (AgentUpdateTrustReliability/Accuracy/Security/Transparency) не
предназначены для прямого запуска -- это sc-агенты: они реагируют на
инициацию sc-действия своего класса в базе знаний. В "боевом" режиме их
инициирует другой агент/сценарий (например, оркестратор подзадач),
вычисливший индикатор по итогам взаимодействия. Этот скрипт даёт
человеку тот же путь вручную -- через websocket-протокол sc-server,
на котором построен и sc-web.

Использование:
    cd ostis_ps_user_ecosystem/scripts
    source ../../interface/sc-web/.venv/bin/activate
    python3 update_trust.py --component rel --trust-level trust_level_my_service --value 1.0

Требует запущенный sc-server (см. ../../start.sh).
"""
import argparse
import sys
import time

from sc_client import client
from sc_client.constants import sc_types
from sc_client.models import ScConstruction, ScIdtfResolveParams, ScLinkContent, ScLinkContentType, ScTemplate

COMPONENT_ACTIONS = {
    "rel": ("action_update_trust_reliability", "t_rel", "индикатор успешности ν_rel"),
    "acc": ("action_update_trust_accuracy", "t_acc", "достигнутая точность q_res"),
    "sec": ("action_update_trust_security", "t_sec", "индикатор отсутствия инцидента ν_sec"),
    "tr": ("action_update_trust_transparency", "t_tr", "индикатор предоставления обоснования ν_tr"),
}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--component", required=True, choices=sorted(COMPONENT_ACTIONS), help="какую компоненту доверия обновить")
    parser.add_argument("--trust-level", required=True, help="системный идентификатор узла trust_level (создаётся, если не существует)")
    parser.add_argument("--value", required=True, type=float, help="значение второго аргумента действия, обычно 0..1")
    parser.add_argument("--url", default="ws://localhost:8090/ws_json", help="адрес sc-server (по умолчанию %(default)s)")
    args = parser.parse_args()

    action_class_idtf, component_label, arg_description = COMPONENT_ACTIONS[args.component]

    client.connect(args.url)
    try:
        concept_trust_level, action_class, action_initiated, rrel_1, rrel_2 = client.resolve_keynodes(
            ScIdtfResolveParams(idtf="concept_trust_level", type=None),
            ScIdtfResolveParams(idtf=action_class_idtf, type=None),
            ScIdtfResolveParams(idtf="action_initiated", type=None),
            ScIdtfResolveParams(idtf="rrel_1", type=None),
            ScIdtfResolveParams(idtf="rrel_2", type=None),
        )
        if not action_class.is_valid():
            print(
                f"ошибка: ключевой узел {action_class_idtf} не найден -- пересоберите БЗ (../../start.sh --rebuild)",
                file=sys.stderr,
            )
            return 1

        (trust_level_addr,) = client.resolve_keynodes(
            ScIdtfResolveParams(idtf=args.trust_level, type=sc_types.NODE_CONST)
        )

        membership_tpl = ScTemplate()
        membership_tpl.triple(concept_trust_level, sc_types.EDGE_ACCESS_VAR_POS_PERM, trust_level_addr)
        if not client.search_by_template(membership_tpl):
            link_constr = ScConstruction()
            link_constr.generate_connector(sc_types.EDGE_ACCESS_CONST_POS_PERM, concept_trust_level, trust_level_addr)
            client.generate_elements(link_constr)
            print(f"создан новый экземпляр concept_trust_level: {args.trust_level}")

        constr = ScConstruction()
        constr.generate_node(sc_types.NODE_CONST, "action")
        constr.generate_connector(sc_types.EDGE_ACCESS_CONST_POS_PERM, action_class, "action")
        constr.generate_link(
            sc_types.LINK_CONST, ScLinkContent(str(args.value), ScLinkContentType.STRING), "value_link"
        )
        constr.generate_connector(sc_types.EDGE_ACCESS_CONST_POS_PERM, "action", trust_level_addr, "arc1")
        constr.generate_connector(sc_types.EDGE_ACCESS_CONST_POS_PERM, rrel_1, "arc1")
        constr.generate_connector(sc_types.EDGE_ACCESS_CONST_POS_PERM, "action", "value_link", "arc2")
        constr.generate_connector(sc_types.EDGE_ACCESS_CONST_POS_PERM, rrel_2, "arc2")
        result = client.generate_elements(constr)
        action_addr = result[constr.get_index("action")]

        init_constr = ScConstruction()
        init_constr.generate_connector(sc_types.EDGE_ACCESS_CONST_POS_PERM, action_initiated, action_addr)
        client.generate_elements(init_constr)

        value_tpl = ScTemplate()
        value_tpl.triple(trust_level_addr, sc_types.EDGE_ACCESS_VAR_POS_PERM, (sc_types.LINK_VAR, "value_link"))
        for _ in range(20):
            results = client.search_by_template(value_tpl)
            if results:
                break
            time.sleep(0.1)
        else:
            print("ошибка: агент не ответил за 2 с -- проверьте логи sc-server", file=sys.stderr)
            return 1

        link_addr = results[0].get("value_link")
        content = client.get_link_content(link_addr)[0]
        print(f"{component_label}({args.trust_level}) <- {arg_description} = {args.value}")
        print(f"{component_label}({args.trust_level}) = {content.data}")
        return 0
    finally:
        client.disconnect()


if __name__ == "__main__":
    sys.exit(main())
